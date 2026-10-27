# First-person rig regression sweep. Run by fp_rig_headless.ps1 (no window), or exec it from the
# editor's Python console while PIE (L_FpsRigTest) is running. It drives the local pawn through
# every state below over a few seconds, then writes Saved/FpRigCheck/result.json, which
# fp_rig_report.py grades.
import json, math, os, time
import unreal

# "full" before reporting / committing; "quick" while iterating. The headless runner sets
# FP_RIG_MODE; when running it by hand, define FP_RIG_MODE = "quick" first.
MODE = globals().get("FP_RIG_MODE") or os.environ.get("FP_RIG_MODE") or "full"

STATES = [
    # name, pitch, crouch, move (forward, right), yaw turn rate (deg/s; per frame made it depend on the editor fps), in quick
    # "_" states are not recorded: stand still first so equip finishes and the grip locks
    # Dropped states and why: README.md "검사 상태 축소". In short, over 64 recorded sweeps
    # (2026-10-27) each dropped state either never failed or only ever failed together with a
    # state that is kept. quick = the kept states that failed alone most often, plus stand_0
    # (the idle reference for "elbows stay down while turning") and stand_dn89 (torso clearance).
    ("_settle", 0, False, (0, 0), 0, True),
    ("stand_up60", 60, False, (0, 0), 0, False),
    ("stand_0", 0, False, (0, 0), 0, True),
    ("stand_dn89", -89, False, (0, 0), 0, True),
    ("crouch_dn85", -85, True, (0, 0), 0, False),
    ("walk_back_dn60", -60, False, (-1, 0), 0, False),
    ("walk_fwd_dn85", -85, False, (1, 0), 0, True),
    ("strafe_l_0", 0, False, (0, -1), 0, True),
    ("strafe_r_0", 0, False, (0, 1), 0, False),
    ("walk_back_crouch_dn60", -60, True, (-1, 0), 0, True),
    # 150 / 1250 deg/s = the former 3 / 25 deg per frame at the usual ~50 fps
    ("turn_in_place_0", 0, False, (0, 0), 150, False),
    # fast mouse turns: the animated upper body lags the view and lifted the left elbow
    ("turn_fast_l_0", 0, False, (0, 0), -1250, True),
]
STATES = [s[:5] for s in STATES if MODE == "full" or s[5]]
WARMUP_FRAMES = 15
SAMPLE_FRAMES = 15
# and at least this long (seconds): transitions such as crouching, and the arm filters (time
# constant ~0.33s), must settle before sampling; 0.6s measured them still converging
WARMUP_SECONDS = 1.5

TORSO = ["pelvis", "spine_01", "spine_02", "spine_03", "thigh_l", "thigh_r"]
GUN_POLY = ["Grip", "Magazine", "Barrel", "Muzzle"]
# visible bones closest to the eyes (head is hidden); too close = camera inside the neck/collar
NEAR_CAMERA = ["neck_01", "neck_02", "spine_05", "clavicle_l", "clavicle_r"]
# shoulder clipped at the screen edge when the upper arm comes too close to the camera
SHOULDERS = ["upperarm_l", "upperarm_r", "upperarm_twist_01_l", "upperarm_twist_01_r"]
OUT_DIR = os.environ.get("FP_RIG_OUT_DIR") or os.path.join(unreal.Paths.project_saved_dir(), "FpRigCheck")
# Every state starts at the PlayerStart tagged FpRigStart (L_FpsRigTest: a 200 m flat slab, no walls
# or ledges). On L_FpsFiringRange_WP Lyra spawns at a random player start and walking backwards hit a
# wall within 5 m from some of them, so results changed per session; PlayerStart_0 there has 8-18 m
# free each way. Falls back to where the pawn stands.
START_TAG = "FpRigStart"
START_PLAYER_START = "PlayerStart_0"


def _world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def _find(world):
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = pc.get_controlled_pawn()
    fp = tp = gun1p = gun3p = None
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if a.get_attach_parent_actor() == pawn:
            for m in a.get_components_by_class(unreal.SkeletalMeshComponent):
                if m.get_name() == "FirstPersonMesh":
                    fp = m
                elif m.get_name() == "MeshComponent":
                    tp = m
        if "Rifle" in a.get_class().get_name():
            for m in a.get_components_by_class(unreal.SkeletalMeshComponent):
                if m.get_editor_property("only_owner_see"):
                    gun1p = m
                else:
                    gun3p = m
    return pc, pawn, fp, tp, gun1p, gun3p


def _seg_dist(q, a, b):
    ab = b - a
    l2 = ab.dot(ab)
    t = 0.0 if l2 < 1e-6 else max(0.0, min(1.0, (q - a).dot(ab) / l2))
    return (a + ab * t - q).length()


def _wrist_bend(mesh, lower, hand):
    fore = (mesh.get_socket_location(hand) - mesh.get_socket_location(lower)).normal()
    hand_x = mesh.get_socket_rotation(hand).get_forward_vector()
    # Manny right-side bones point -X toward the child.
    if hand.endswith("_r"):
        hand_x = hand_x * -1.0
    return math.degrees(math.acos(max(-1.0, min(1.0, fore.dot(hand_x)))))


def _barrel_deg(view, gun):
    d = view.inverse_transform_location(gun.get_socket_location("Muzzle")) - view.inverse_transform_location(gun.get_socket_location("Barrel"))
    return [round(math.degrees(math.atan2(d.y, d.x)), 2), round(math.degrees(math.atan2(d.z, d.x)), 2), 0.0]


def _grip_locked(world):
    pc, pawn, fp, tp, gun, gun3p = _find(world)
    return bool(gun) and str(gun.get_owner().get_editor_property("root_component").get_attach_socket_name()) == "hand_r"


def _chest_yaw(view, mesh):
    a = view.inverse_transform_location(mesh.get_socket_location("upperarm_l"))
    b = view.inverse_transform_location(mesh.get_socket_location("upperarm_r"))
    return round(math.degrees(math.atan2(a.y - b.y, a.x - b.x)), 1)


FINGER_JOINTS = [f + "_" + n for f in ("thumb",) for n in ("02", "03")] + [f + "_" + n for f in ("index", "middle", "ring", "pinky") for n in ("01", "02", "03")]


def _fingers(mesh):
    out = {}
    for side in ("l", "r"):
        hand = mesh.get_socket_transform("hand_" + side)
        for j in FINGER_JOINTS:
            out[j + "_" + side] = _v(hand.inverse_transform_location(mesh.get_socket_location(j + "_" + side)))
    return out

def _v(v):
    return [round(v.x, 2), round(v.y, 2), round(v.z, 2)]


def _sample(world):
    pc, pawn, fp, tp, gun, gun3p = _find(world)
    cm = pc.player_camera_manager
    view = unreal.Transform(cm.get_camera_location(), cm.get_camera_rotation(), unreal.Vector(1, 1, 1))
    gun_xf = gun.get_socket_transform("Grip")
    poly = [gun.get_socket_location(s) for s in GUN_POLY]
    clear = {}
    for b in TORSO:
        q = fp.get_socket_location(b)
        clear[b] = min(_seg_dist(q, poly[i], poly[i + 1]) for i in range(len(poly) - 1))
    feet = max((fp.get_socket_location(b) - tp.get_socket_location(b)).length() for b in ("foot_l", "foot_r", "ball_l", "ball_r"))
    cam = cm.get_camera_location()
    near = min((fp.get_socket_location(b) - cam).length() for b in NEAR_CAMERA)
    shoulder = min((fp.get_socket_location(b) - cam).length() for b in SHOULDERS)
    return {
        "torso_clearance_cm": round(min(clear.values()), 2),
        "torso_closest_bone": min(clear, key=clear.get),
        "feet_fp_vs_shadow_cm": round(feet, 2),
        "body_near_camera_cm": round(near, 2),
        "shoulder_near_camera_cm": round(shoulder, 2),
        "muzzle_view": _v(view.inverse_transform_location(gun.get_socket_location("Muzzle"))),
        "barrel_view_deg": _barrel_deg(view, gun),
        "hand_l_in_gun": _v(gun_xf.inverse_transform_location(fp.get_socket_location("hand_l"))),
        "hand_r_in_gun": _v(gun_xf.inverse_transform_location(fp.get_socket_location("hand_r"))),
        "wrist_bend_l_deg": round(_wrist_bend(fp, "lowerarm_l", "hand_l"), 1),
        "wrist_bend_r_deg": round(_wrist_bend(fp, "lowerarm_r", "hand_r"), 1),
        # same pose on the 3P (shadow) mesh: the rifle pistol grip bends the right wrist by design
        "wrist_bend_l_3p_deg": round(_wrist_bend(tp, "lowerarm_l", "hand_l"), 1),
        "wrist_bend_r_3p_deg": round(_wrist_bend(tp, "lowerarm_r", "hand_r"), 1),
        # shoulder line yaw in view: 1P (corrected) vs 3P (animation)
        "chest_yaw_1p": _chest_yaw(view, fp),
        "chest_yaw_3p": _chest_yaw(view, tp),
        "elbow_l_view": _v(view.inverse_transform_location(fp.get_socket_location("lowerarm_l"))),
        "elbow_r_view": _v(view.inverse_transform_location(fp.get_socket_location("lowerarm_r"))),
        # the arm's shape around the gun: moving with the gun is sway, moving against it is the elbow bob
        "elbow_l_in_gun": _v(gun_xf.inverse_transform_location(fp.get_socket_location("lowerarm_l"))),
        "elbow_r_in_gun": _v(gun_xf.inverse_transform_location(fp.get_socket_location("lowerarm_r"))),
        # finger joints in their hand's space (1P, and the 3P animation for comparison)
        "fingers_1p": _fingers(fp),
        "fingers_3p": _fingers(tp),
    }


def _static_checks(world):
    pc, pawn, fp, tp, gun, gun3p = _find(world)
    p = lambda m, n: m.get_editor_property(n)
    FP = unreal.FirstPersonPrimitiveType
    legs = [c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == "FpsWorldLayerLegs"]
    cam = pawn.get_component_by_class(unreal.CameraComponent)
    scale = p(cam, "first_person_scale") if cam and p(cam, "enable_first_person_scale") else 1.0
    radius = pawn.get_component_by_class(unreal.CapsuleComponent).get_scaled_capsule_radius()
    cm = pc.player_camera_manager
    view = unreal.Transform(cm.get_camera_location(), cm.get_camera_rotation(), unreal.Vector(1, 1, 1))
    muzzle = view.inverse_transform_location(gun.get_socket_location("Muzzle")).length()
    return {
        # owner shadow comes from the 3P body (engine World Space Representation)
        "body3p_world_space_representation_casts_shadow": p(tp, "first_person_primitive_type") == FP.WORLD_SPACE_REPRESENTATION and p(tp, "cast_shadow"),
        "body1p_no_shadow": not p(fp, "cast_shadow"),
        "gun3p_hidden_no_owner_shadow": bool(gun3p) and p(gun3p, "owner_no_see"),
        "gun1p_no_shadow": not p(gun, "cast_shadow"),
        "head_hidden_1p": fp.is_bone_hidden_by_name("head"),
        # gun through wall / own thigh while turning: arms, upper body and gun in the first person layer
        "body1p_first_person_layer": p(fp, "first_person_primitive_type") == FP.FIRST_PERSON,
        "gun1p_first_person_layer": p(gun, "first_person_primitive_type") == FP.FIRST_PERSON,
        "legs_world_layer": len(legs) == 1 and p(legs[0], "first_person_primitive_type") == FP.NONE and legs[0].is_bone_hidden_by_name("spine_01") and fp.is_bone_hidden_by_name("thigh_l") and fp.is_bone_hidden_by_name("thigh_r"),
        "legs_follow_1p_pose": len(legs) == 1 and (legs[0].get_socket_location("foot_l") - fp.get_socket_location("foot_l")).length() < 0.5,
        # scaled toward the camera, the muzzle stays inside the capsule (no wall clipping)
        "gun_inside_capsule_when_scaled": muzzle * scale < radius,
    }


class _Sweep:
    def __init__(self):
        world = _world()
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        self.yaw = pc.get_control_rotation().yaw
        self.start = pc.get_controlled_pawn().get_actor_location()
        for start in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart):
            if START_TAG in [str(t) for t in start.tags] or start.get_name() == START_PLAYER_START:
                self.yaw = start.get_actor_rotation().yaw
                self.start = start.get_actor_location()
        self.state = 0
        self.frame = 0
        self.state_time = 0.0
        # static checks run once the character has settled (the weapon may not exist yet at PIE start)
        self.results = {"static": None, "states": {}, "mode": MODE}
        self.wall_start = time.time()
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def _enter(self, world):
        name, pitch, crouch, move, turn = STATES[self.state]
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        pawn = pc.get_controlled_pawn()
        pawn.teleport(self.start, unreal.Rotator(0, 0, self.yaw))
        pc.set_control_rotation(unreal.Rotator(roll=0, pitch=pitch, yaw=self.yaw))
        if crouch:
            pawn.crouch()
        else:
            pawn.un_crouch()

    def tick(self, dt):
        try:
            world = _world()
            if world is None:
                return self.finish("PIE ended")
            name, pitch, crouch, move, turn = STATES[self.state]
            if self.frame == 0:
                self._enter(world)
                self.state_time = 0.0
            # game time, so a fixed time step (-UseFixedTimeStep, see fp_rig_headless.ps1) runs faster than real time
            self.state_time += unreal.GameplayStatics.get_world_delta_seconds(world)
            warm = self.frame >= WARMUP_FRAMES and self.state_time >= WARMUP_SECONDS
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            pc.set_control_rotation(unreal.Rotator(roll=0, pitch=pitch, yaw=self.yaw + turn * self.state_time))
            if move != (0, 0):
                yaw_rot = unreal.Rotator(roll=0, pitch=0, yaw=self.yaw)
                pc.get_controlled_pawn().add_movement_input(yaw_rot.get_forward_vector() * move[0] + yaw_rot.get_right_vector() * move[1], 1.0, True)
            if warm and not name.startswith("_"):
                if self.results["static"] is None:
                    self.results["static"] = _static_checks(world)
                self.results["states"].setdefault(name, []).append(_sample(world))
            self.frame += 1
            if warm:
                self.samples = getattr(self, "samples", 0) + 1
            if warm and self.samples >= SAMPLE_FRAMES:
                # settle until the 1P weapon hangs off the hand (grip captured), at most ~20s of frames
                if name == "_settle" and not _grip_locked(world) and self.frame < 600:
                    return
                self.frame = 0
                self.samples = 0
                self.state += 1
                if self.state >= len(STATES):
                    return self.finish(None)
        except Exception as e:
            self.finish(repr(e))

    def finish(self, error):
        unreal.unregister_slate_post_tick_callback(self.handle)
        self.results["error"] = error
        self.results["wall_seconds"] = round(time.time() - self.wall_start, 1)
        os.makedirs(OUT_DIR, exist_ok=True)
        with open(os.path.join(OUT_DIR, "result.json"), "w") as f:
            json.dump(self.results, f)
        try:
            world = _world()
            if world:
                pc = unreal.GameplayStatics.get_player_controller(world, 0)
                pc.get_controlled_pawn().un_crouch()
        except Exception:
            pass
        global _FP_RIG_SWEEP
        _FP_RIG_SWEEP = None
        # the headless runner (fp_rig_headless.py) grades and quits from here
        on_done = globals().get("FP_RIG_ON_DONE")
        if on_done:
            on_done(error)


_FP_RIG_SWEEP = _Sweep()
print("fp rig sweep started:", len(STATES), "states")
