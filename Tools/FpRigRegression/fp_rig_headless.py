# Headless driver for the first-person rig sweep. Started by fp_rig_headless.ps1 through
# -ExecCmds="py <this file>".
# Not -ExecutePythonScript: that quits the editor as soon as the script returns, before PIE runs.
# Loads the test level, starts PIE, runs fp_rig_check.py, grades it with fp_rig_report.py,
# writes <out>/report.txt and quits the editor. No window, no polling by the caller.
import os, time
import unreal

HERE = os.environ["FP_RIG_DIR"]
LEVEL = os.environ.get("FP_RIG_LEVEL", "/FpsMaps/Maps/L_FpsRigTest")
# pawn spawn + equip after PIE start; the sweep itself has its own settle state
PIE_TIMEOUT_S = 120.0


def _log(*a):
    unreal.log("[fp_rig_headless] " + " ".join(str(x) for x in a))


def _run_file(name, scope):
    with open(os.path.join(HERE, name), encoding="utf-8") as f:
        exec(compile(f.read(), name, "exec"), scope)


class _Driver:
    def __init__(self):
        self.phase = "load"
        self.t0 = time.time()
        self.scope = {"__name__": "fp_rig", "FP_RIG_ON_DONE": self.on_sweep_done}
        self.handle = unreal.register_slate_post_tick_callback(self.tick)

    def tick(self, dt):
        try:
            les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
            ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
            if self.phase == "load":
                if not les.load_level(LEVEL):
                    return self.quit("cannot load " + LEVEL)
                les.editor_request_begin_play()
                self.phase, self.t_pie = "pie", time.time()
                _log("PIE requested", LEVEL)
            elif self.phase == "pie":
                world = ues.get_game_world()
                pc = world and unreal.GameplayStatics.get_player_controller(world, 0)
                if pc and pc.get_controlled_pawn():
                    _log("pawn ready after %.1fs" % (time.time() - self.t_pie))
                    self.phase = "sweep"
                    _run_file("fp_rig_check.py", self.scope)
                elif time.time() - self.t_pie > PIE_TIMEOUT_S:
                    return self.quit("no pawn after %ds" % PIE_TIMEOUT_S)
            elif self.phase == "sweep":
                self._always_refresh_bones(ues.get_game_world())
            elif self.phase == "report":
                _run_file("fp_rig_report.py", self.scope)
                self.quit(None)
        except Exception as e:
            self.quit(repr(e))

    # With -nullrhi nothing is ever rendered, and a skeletal mesh on the default AlwaysTickPose
    # ticks its animation but does not refresh its bones, so every socket read returned the
    # un-IK'd pose (38 FAIL that a rendered run did not have). Rescanned every 25 ticks because
    # cosmetics and the local weapon are spawned after the pawn. Attribute assignment, not
    # set_editor_property: that reruns construction scripts on PIE actors and drops cosmetics.
    def _always_refresh_bones(self, world):
        self.ticks = getattr(self, "ticks", 0) + 1
        if world is None or self.ticks % 25 != 1:
            return
        always = unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
            for m in a.get_components_by_class(unreal.SkeletalMeshComponent):
                if m.visibility_based_anim_tick_option != always:
                    m.visibility_based_anim_tick_option = always

    def on_sweep_done(self, error):
        _log("sweep done", error)
        self.phase = "report"

    def quit(self, error):
        unreal.unregister_slate_post_tick_callback(self.handle)
        _log("total %.1fs" % (time.time() - self.t0), "error:", error)
        if error:
            out = os.environ.get("FP_RIG_OUT_DIR") or os.path.join(unreal.Paths.project_saved_dir(), "FpRigCheck")
            os.makedirs(out, exist_ok=True)
            with open(os.path.join(out, "report.txt"), "w") as f:
                f.write("FAIL headless driver  %s\n1 FAIL" % error)
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
        unreal.SystemLibrary.quit_editor()


_FP_RIG_DRIVER = _Driver()
