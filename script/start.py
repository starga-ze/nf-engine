import os
import sys
import subprocess

from script.utils import (
    ROOT_DIR,
    BUILD_DIR,
    run_cmd,
)

BUILD_BIN_DIR = os.path.join(BUILD_DIR, "bin")

SERVER_BIN = os.path.join(BUILD_BIN_DIR, "nf-serverd")
MGMT_BIN = os.path.join(BUILD_BIN_DIR, "nf-mgmtd")

SERVER_INSTALL = "/usr/bin/nf-serverd"
MGMT_INSTALL = "/usr/bin/nf-mgmtd"

SCRIPT_DIR = os.path.dirname(__file__)

SERVER_SERVICE_SRC = os.path.join(SCRIPT_DIR, "nf-serverd.service")
MGMT_SERVICE_SRC = os.path.join(SCRIPT_DIR, "nf-mgmtd.service")

SYSTEMD_DIR = "/etc/systemd/system"


def check_binaries():
    print("[*] Checking built binaries...")

    for path in [SERVER_BIN, MGMT_BIN]:
        if not os.path.isfile(path):
            print(f"[ERROR] Binary not found: {path}")
            print("[*] Please run build first.")
            sys.exit(1)


def stop_server_if_running():
    subprocess.run(
        ["systemctl", "stop", "nf-serverd"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )


def install_binaries():
    run_cmd(["cp", SERVER_BIN, SERVER_INSTALL],
            msg="Installing nf-serverd")

    run_cmd(["cp", MGMT_BIN, MGMT_INSTALL],
            msg="Installing nf-mgmtd")

    run_cmd(["chmod", "755", SERVER_INSTALL])
    run_cmd(["chmod", "755", MGMT_INSTALL])


def install_service_files():
    run_cmd(["cp", SERVER_SERVICE_SRC, SYSTEMD_DIR],
            msg="Installing nf-serverd.service")

    run_cmd(["cp", MGMT_SERVICE_SRC, SYSTEMD_DIR],
            msg="Installing nf-mgmtd.service")


def reload_systemd():
    run_cmd(["systemctl", "daemon-reload"],
            msg="Reloading systemd")


def enable_services():
    run_cmd(["systemctl", "enable", "nf-serverd"], msg=None)
    run_cmd(["systemctl", "enable", "nf-mgmtd"], msg=None)


def restart_server():
    run_cmd(["systemctl", "restart", "nf-serverd"],
            msg="Restarting nf-serverd")


def run():
    print("[*] Starting nf-engine services...")

    check_binaries()
    stop_server_if_running()

    install_binaries()
    install_service_files()

    reload_systemd()
    enable_services()
    restart_server()

    subprocess.run(["systemctl", "status", "nf-serverd", "--no-pager"])

    print("[*] Done. Services are running.")


if __name__ == "__main__":
    run()

