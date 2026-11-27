import subprocess

Import("env")

project_dir = env.subst("$PROJECT_DIR")

try:
    revision = subprocess.check_output(
        ["git", "rev-parse", "--short=7", "HEAD"],
        cwd=project_dir,
        stderr=subprocess.DEVNULL,
        universal_newlines=True,
    ).strip()
    dirty = subprocess.check_output(
        [
            "git",
            "status",
            "--porcelain",
            "--untracked-files=normal",
            "--",
            "src",
            "platformio.ini",
            "scripts/build_revision.py",
        ],
        cwd=project_dir,
        stderr=subprocess.DEVNULL,
        universal_newlines=True,
    ).strip()
    if dirty:
        revision += "*"
except (OSError, subprocess.CalledProcessError):
    revision = "unknown"
    print("Warning: unable to determine the Git build revision")

env.Append(CPPDEFINES=[("APP_BUILD_REVISION", '\\"{}\\"'.format(revision))])
