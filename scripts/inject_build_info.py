# PlatformIO pre-build script (Nexus 176cc276 splash screen).
#
# The splash screen shows a short git SHA so a badge holder can tell exactly
# which build they're running. Per the ticket: "a boot screen that reports a
# version the badge is not running is worse than no boot screen" - so this
# has to be the real SHA of the checkout being compiled RIGHT NOW, derived at
# build time, never typed as a literal anywhere in source. See
# sim/build.sh for the simulator's equivalent of this same injection.
Import("env")

import subprocess

try:
    sha = subprocess.check_output(
        ["git", "rev-parse", "--short=8", "HEAD"],
        stderr=subprocess.DEVNULL,
    ).decode().strip()
    if subprocess.check_output(["git", "status", "--porcelain"]).strip():
        sha += "-dirty"
except Exception:
    sha = "unknown"

env.Append(CPPDEFINES=[("BP_GIT_SHA", '\\"%s\\"' % sha)])
print("[inject_build_info] BP_GIT_SHA=%s" % sha)
