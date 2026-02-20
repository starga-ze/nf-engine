import os
import subprocess
import sys
import shutil

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))

BUILD_DIR = os.path.join(ROOT_DIR, "build")
CERT_DIR = os.path.join(ROOT_DIR, "cert")

EXEC = os.path.join(BUILD_DIR, "nf-serverd")

INSTALL_ROOT = os.path.join(ROOT_DIR, "3rd_party", "install")

OPENSSL_VERSION = "3.2.0"
OPENSSL_DIR = os.path.join(ROOT_DIR, "3rd_party", "openssl")
OPENSSL_INSTALL = os.path.join(INSTALL_ROOT, "openssl")
OPENSSL_TAR = os.path.join(OPENSSL_DIR, f"openssl-{OPENSSL_VERSION}.tar.gz")
OPENSSL_SRC_PATH = os.path.join(OPENSSL_DIR, f"openssl-{OPENSSL_VERSION}")

SPDLOG_DIR = os.path.join(ROOT_DIR, "3rd_party", "spdlog")
SPDLOG_INSTALL = os.path.join(INSTALL_ROOT, "spdlog")

BOOST_VERSION = "1.84.0"
BOOST_VERSION_UNDERSCORE = BOOST_VERSION.replace(".", "_")
BOOST_DIR = os.path.join(ROOT_DIR, "3rd_party", "boost")
BOOST_INSTALL = os.path.join(INSTALL_ROOT, "install")
BOOST_TAR = os.path.join(BOOST_DIR, f"boost_{BOOST_VERSION_UNDERSCORE}.tar.gz")
BOOST_SRC_PATH = os.path.join(BOOST_DIR, f"boost_{BOOST_VERSION_UNDERSCORE}")

NUM_CORES = os.cpu_count() or 1
MAKE_JOBS = f"-j{NUM_CORES}"


def run_cmd(cmd, cwd=ROOT_DIR, msg=None):
    if msg:
        print(f"[*] {msg}...")

    try:
        result = subprocess.run(cmd, cwd=cwd)

        if result.returncode != 0:
            print(f"[Error] Command failed: {' '.join(cmd)}")
            sys.exit(result.returncode)

    except FileNotFoundError:
        print(f"[Error] Command not found: {cmd[0]}")
        sys.exit(1)

    except Exception as e:
        print(f"[Error] Unexpected error: {e}")
        sys.exit(1)

