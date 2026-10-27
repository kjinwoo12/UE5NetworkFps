# Grades Saved/FpRigCheck/result.json written by fp_rig_check.py. Run by fp_rig_headless.py,
# or exec it from the editor's Python console (PIE may be stopped). Prints PASS/FAIL per check
# and writes Saved/FpRigCheck/report.txt. Every limit comes from a bug that was seen once;
# do not loosen one to make a fix pass.
import json, os
import unreal

OUT_DIR = os.environ.get("FP_RIG_OUT_DIR") or os.path.join(unreal.Paths.project_saved_dir(), "FpRigCheck")
LIMITS = {
    # look down: gun through belly/hips (reported twice)
    "torso_clearance_cm_min": 14.0,
    # owner shadow feet vs 1P feet
    "feet_static_cm_max": 1.0,
    "feet_moving_cm_max": 3.0,
    # bent support wrist (reported twice); absolute, the support hand cups the handguard
    "wrist_bend_l_deg_max": 30.0,
    # either wrist must not bend more than the same frame of the 3P animation
    # (the rifle pistol grip bends the right wrist 30-70 deg in the animation itself)
    "wrist_bend_vs_3p_deg_max": 10.0,
    # hands sliding on the gun within one state (walking backwards, turning)
    "hand_in_gun_range_cm_max": 1.5,
    # standing still the gun must not drift on screen (true first person lets it sway with the
    # body while moving or changing pitch, like the third-person animation)
    "muzzle_view_static_range_cm_max": 2.0,
    # the barrel stays aligned with the view; since 2026-10-20 the weapon follows the body within
    # WeaponSwayMaxAngle (6 deg) each way by design (a locked gun made the arms jerk)
    "barrel_view_range_deg_max": 13.0,
    # elbows high / in front of the camera; arms filling the screen
    "elbow_view_z_max": -8.0,
    # camera inside the neck/collar (true first person)
    "body_near_camera_cm_min": 14.0,
    # shoulder mesh clipped at the left edge of the screen (upper arm bone centre to camera)
    "shoulder_near_camera_cm_min": 20.0,
    # fast turns lifted the elbow above where it sits standing still (stand_0)
    "elbow_rise_vs_idle_cm_max": 1.5,
    # elbows jerking up and down while walking / turning (visible elbows only)
    "elbow_bob_cm_max": 1.5,
    # elbows moving against the gun while walking / turning ("the gun moves, but the elbow still bobs")
    "elbow_vs_gun_range_cm_max": 1.0,
    # finger joints opening/closing on the grip within one state (thumb flapping while walking)
    "finger_range_cm_max": 0.3,
}


def _rng(samples, key):
    out = 0.0
    for i in range(3):
        vals = [s[key][i] for s in samples]
        out = max(out, max(vals) - min(vals))
    return out


def main():
    with open(os.path.join(OUT_DIR, "result.json")) as f:
        r = json.load(f)
    lines, fails = [], 0

    def check(name, ok, detail):
        nonlocal fails
        fails += 0 if ok else 1
        lines.append(("PASS " if ok else "FAIL ") + name + "  " + detail)

    if r.get("error"):
        check("sweep finished", False, r["error"])
    for k, v in r["static"].items():
        check("static " + k, bool(v), str(v))

    every = [s for ss in r["states"].values() for s in ss]
    for name, ss in r["states"].items():
        moving = "walk" in name or "strafe" in name
        c = min(s["torso_clearance_cm"] for s in ss)
        bone = min(ss, key=lambda s: s["torso_clearance_cm"])["torso_closest_bone"]
        check(name + " torso clear", c >= LIMITS["torso_clearance_cm_min"], "%.1fcm (%s)" % (c, bone))
        f = max(s["feet_fp_vs_shadow_cm"] for s in ss)
        lim = LIMITS["feet_moving_cm_max" if moving else "feet_static_cm_max"]
        check(name + " feet on shadow", f <= lim, "%.2fcm" % f)
        # absolute limit, unless the 3P animation itself bends the support wrist more in that frame
        wl = max(s["wrist_bend_l_deg"] - max(0.0, s["wrist_bend_l_3p_deg"] - LIMITS["wrist_bend_l_deg_max"]) for s in ss)
        dl = max(s["wrist_bend_l_deg"] - s["wrist_bend_l_3p_deg"] for s in ss)
        dr = max(s["wrist_bend_r_deg"] - s["wrist_bend_r_3p_deg"] for s in ss)
        ok = wl <= LIMITS["wrist_bend_l_deg_max"] and max(dl, dr) <= LIMITS["wrist_bend_vs_3p_deg_max"]
        check(name + " wrists", ok, "L %.0f deg, vs 3P L %+.0f R %+.0f deg" % (wl, dl, dr))
        if "fingers_1p" in ss[0]:
            def finger_range(key):
                return max(max(max(s[key][j][i] for s in ss) - min(s[key][j][i] for s in ss) for i in range(3)) for j in ss[0][key])
            fr = finger_range("fingers_1p")
            worst = max(ss[0]["fingers_1p"], key=lambda j: max(max(s["fingers_1p"][j][i] for s in ss) - min(s["fingers_1p"][j][i] for s in ss) for i in range(3)))
            check(name + " fingers keep the grip", fr <= LIMITS["finger_range_cm_max"], "%.2fcm (%s; 3P anim %.2fcm)" % (fr, worst, finger_range("fingers_3p")))
        nc = min(s["body_near_camera_cm"] for s in ss)
        check(name + " camera outside body", nc >= LIMITS["body_near_camera_cm_min"], "%.1fcm" % nc)
        sc = min(s["shoulder_near_camera_cm"] for s in ss)
        check(name + " shoulders away from camera", sc >= LIMITS["shoulder_near_camera_cm_min"], "%.1fcm" % sc)
        ez = max(max(s["elbow_l_view"][2], s["elbow_r_view"][2]) for s in ss)
        check(name + " elbows low", ez <= LIMITS["elbow_view_z_max"], "view z %.1f" % ez)
        hl = _rng(ss, "hand_l_in_gun")
        hr = _rng(ss, "hand_r_in_gun")
        check(name + " hands stay on gun", max(hl, hr) <= LIMITS["hand_in_gun_range_cm_max"], "L %.2f R %.2f cm" % (hl, hr))
        if not moving and "turn" not in name:
            mz = _rng(ss, "muzzle_view")
            check(name + " gun still on screen", mz <= LIMITS["muzzle_view_static_range_cm_max"], "%.2fcm" % mz)

    idle = r["states"].get("stand_0")
    if idle:
        for name, ss in r["states"].items():
            if not name.startswith("turn"):
                continue
            # only an elbow in front of the camera can be seen rising (the right one sits below/behind it)
            rises = [max(s[k][2] for s in ss) - max(i[k][2] for i in idle)
                     for k in ("elbow_l_view", "elbow_r_view") if max(s[k][0] for s in ss) > 10.0]
            rise = max(rises) if rises else 0.0
            check(name + " elbows stay down while turning", rise <= LIMITS["elbow_rise_vs_idle_cm_max"], "%+.1fcm vs stand_0" % rise)

    for name, ss in r["states"].items():
        if not ("walk" in name or "strafe" in name or "turn" in name):
            continue
        bobs = [max(s[k][2] for s in ss) - min(s[k][2] for s in ss)
                for k in ("elbow_l_view", "elbow_r_view") if max(s[k][0] for s in ss) > 10.0]
        bob = max(bobs) if bobs else 0.0
        check(name + " elbows steady", bob <= LIMITS["elbow_bob_cm_max"], "%.1fcm up/down" % bob)
        if "elbow_l_in_gun" in ss[0]:
            # visible elbows only, as for the other elbow checks (x <= 10 is beside / behind the camera)
            eg = max([_rng(ss, k + "_in_gun") for k in ("elbow_l", "elbow_r") if max(s[k + "_view"][0] for s in ss) > 10.0] or [0.0])
            check(name + " elbows move with the gun", eg <= LIMITS["elbow_vs_gun_range_cm_max"], "%.1fcm vs gun" % eg)

    bd = _rng(every, "barrel_view_deg")
    check("all states barrel aligned with view", bd <= LIMITS["barrel_view_range_deg_max"], "%.2f deg" % bd)

    lines.append("%d FAIL" % fails if fails else "ALL PASS")
    text = "\n".join(lines)
    with open(os.path.join(OUT_DIR, "report.txt"), "w") as f:
        f.write(text)
    print(text)


main()
