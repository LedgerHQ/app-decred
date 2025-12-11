from ragger.backend import RaisePolicy
from ragger.navigator.navigation_scenario import NavigateWithScenario
from binascii import hexlify
from pathlib import Path
from inspect import currentframe
from conftest import ROOT_SCREENSHOT_PATH


def test_addr_display(scenario_navigator: NavigateWithScenario):
    packets = [
        "058000002c8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    with scenario_navigator.backend.exchange_async_raw(
            data=bytearray.fromhex(packets[0])) as r:
        scenario_navigator.address_review_approve()


def test_addr_display_reject(scenario_navigator):
    packets = [
        "058000002c8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    scenario_navigator.backend.raise_policy = RaisePolicy.RAISE_NOTHING
    with scenario_navigator.backend.exchange_async_raw(
            data=bytearray.fromhex(packets[0])) as r:
        scenario_navigator.address_review_reject()
    assert (scenario_navigator.backend.last_async_response.status == 0x6985)
