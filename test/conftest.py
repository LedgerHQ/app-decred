import os
# from turtle import back
import pytest
from pathlib import Path

# from venvtron.lib.python3.8.site-packages.mnemonic.mnemonic import Mnemonic
from ragger import Firmware
from ragger.navigator import NanoNavigator, FatstacksNavigator
from ragger.backend import SpeculosBackend, LedgerCommBackend
from time import sleep

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

# This variable is needed for Speculos only (physical tests need the application to be already installed)
APPS_DIRECTORY = (Path(__file__).parent / "elfs").resolve()
APP_PREFIX = "decred"

# This variable will be useful in tests to implement different behavior depending on the firmware
FIRMWARES = [Firmware('nanos', '2.1'),
             Firmware('nanosp', '1.0.3'),
             Firmware('nanox', '2.0.2'),
             Firmware('fat', '1.0')]

BACKENDS = ["speculos", "ledgercomm", "ledgerwallet"]

MNEMONIC = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"

# Glue to call every test that depends on the firmware once for each required firmware
def pytest_generate_tests(metafunc):    
    if "firmware" in metafunc.fixturenames:
        fw_list = []
        ids = []
        # First pass: enable only demanded firmwares
        for fw in FIRMWARES:
            if metafunc.config.getoption(fw.device):
                fw_list.append(fw)
                ids.append(fw.device + " " + fw.version)
        # Second pass if no specific firmware demanded: add them all
        if not fw_list:
            for fw in FIRMWARES:
                fw_list.append(fw)
                ids.append(fw.device + " " + fw.version)
        metafunc.parametrize("firmware", fw_list, ids=ids, scope="session")

# adding a pytest CLI option "--backend"
def pytest_addoption(parser):
    parser.addoption("--backend", action="store", default="speculos")
    parser.addoption("--golden_run", action="store_true", default=False)
    # Enable ussing --'device' in the pytest command line to restrict testing to specific devices
    for fw in FIRMWARES:
        parser.addoption("--"+fw.device, action="store_true", help="run on specified device only")

# accessing the value of the "--backend" option as a fixture
@pytest.fixture(scope="session")
def backend(pytestconfig):
    return pytestconfig.getoption("backend")

def app_path_from_app_name(app_dir, app_name: str, device: str) -> Path:
    assert app_dir.is_dir(), f"{app_dir} is not a directory"
    app_path = app_dir / (app_name + "_" + device + ".elf")
    assert app_path.is_file(), f"{app_path} must exist"
    return app_path

def prepare_speculos_args(firmware):
    speculos_args = ["--model", firmware.device, "--sdk", firmware.version, "--seed", MNEMONIC]
    # Uncomment line below to enable display
    speculos_args += ["--display", "qt"]
    app_path = app_path_from_app_name(APPS_DIRECTORY, APP_PREFIX, firmware.device)
    return ([app_path], {"args": speculos_args})

# Depending on the "--backend" option value, a different backend is
# instantiated, and the tests will either run on Speculos or on a physical
# device depending on the backend
def create_backend(backend: str, firmware: Firmware):
    # if backend.lower() == "ledgercomm":
    #     return LedgerCommBackend(firmware, interface="hid")
    # elif backend.lower() == "ledgerwallet":
    #     return LedgerWalletBackend(firmware)
    if backend.lower() == "speculos":
        args, kwargs = prepare_speculos_args(firmware)
        return SpeculosBackend(*args, firmware, **kwargs)
    else:
        raise ValueError(f"Backend '{backend}' is unknown. Valid backends are: {BACKENDS}")


@pytest.fixture(scope="session")
def golden_run(pytestconfig):
    return pytestconfig.getoption("golden_run")

@pytest.fixture(scope="session")
def navigator(client, firmware, golden_run):
    if firmware.device.startswith("fat"):
        return FatstacksNavigator(client, firmware, golden_run)
    elif firmware.device.startswith("nano"):
        return NanoNavigator(client, firmware, golden_run)
    else:
        raise ValueError(f"Device '{firmware.device}' is unsupported.")


# This final fixture will return the properly configured backend client, to be used in tests
@pytest.fixture(scope="session")
def client(backend, firmware):
    with create_backend(backend, firmware) as b:
        yield b

