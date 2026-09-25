# PlatformIO pre-build script (Nexus 176cc276 splash screen).
#
# The splash screen shows a short git SHA so a badge holder can tell exactly
# which build they're running. Per the ticket: "a boot screen that reports a
# version the badge is not running is worse than no boot screen" - so this
# has to be the real SHA of the checkout being compiled RIGHT NOW, derived at
# build time, never typed as a literal anywhere in source. See
# sim/build.sh for the simulator's equivalent of this same injection.
#
# Same requirement applies to the Marauder version shown next to it: it must
# be read out of upstream's own configs.h, not hand-copied into our shim
# (that hand-copy is exactly what QA caught on Nexus 176cc276 - see
# include/marauder_config.h for the resulting #define).
Import("env")

import os
import re
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

project_dir = env.subst("$PROJECT_DIR")
upstream_configs_h = os.path.join(
    project_dir, "esp32marauder-upstream", "esp32_marauder", "configs.h"
)
with open(upstream_configs_h) as f:
    upstream_text = f.read()
match = re.search(r'#define\s+MARAUDER_VERSION\s+"([^"]+)"', upstream_text)
if not match:
    raise RuntimeError(
        "inject_build_info: could not find MARAUDER_VERSION in %s - "
        "upstream configs.h format changed, splash version block would go "
        "stale silently if this build continued" % upstream_configs_h
    )
marauder_version = match.group(1)

env.Append(CPPDEFINES=[("BP_MARAUDER_VERSION_UPSTREAM", '\\"%s\\"' % marauder_version)])
print("[inject_build_info] BP_MARAUDER_VERSION_UPSTREAM=%s" % marauder_version)
