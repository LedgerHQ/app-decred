from ragger.navigator import NavInsID
from ragger.backend import RaisePolicy
from binascii import hexlify
from pathlib import Path
from inspect import currentframe
from conftest import ROOT_SCREENSHOT_PATH


def test_addr_display(backend, firmware, navigator):
    packets = [
        "058000002c8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(
                ROOT_SCREENSHOT_PATH,
                test_name,
                [
                    NavInsID.TAPPABLE_CENTER_TAP, NavInsID.TAPPABLE_CENTER_TAP,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_TAP,
                    NavInsID.WAIT_FOR_HOME_SCREEN
                ],
            )
        else:
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)


def test_addr_display_reject(backend, firmware, navigator):
    packets = [
        "058000002c8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(
                ROOT_SCREENSHOT_PATH,
                test_name,
                [
                    NavInsID.TAPPABLE_CENTER_TAP,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL,
                    NavInsID.WAIT_FOR_HOME_SCREEN
                ],
            )
        else:
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Reject",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
    assert (backend.last_async_response.status == 0x6985)


def test_addr_display_unusual_path(backend, firmware, navigator):
    packets = [
        "058000002b8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.USE_CASE_CHOICE_REJECT, NavInsID.USE_CASE_REVIEW_TAP,
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_TAP,
                NavInsID.WAIT_FOR_HOME_SCREEN
            ])
        elif firmware.device == "nanos":
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
            ])
        else:
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
            ])


def test_addr_display_unusual_path_reject_path(backend, firmware, navigator):
    packets = [
        "058000002b8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.USE_CASE_CHOICE_CONFIRM, NavInsID.WAIT_FOR_HOME_SCREEN
            ])
        else:
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
            ])
    assert (backend.last_async_response.status == 0x6985)


def test_addr_display_unusual_path_reject_address(backend, firmware,
                                                  navigator):
    packets = [
        "058000002b8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    test_name = Path(currentframe().f_code.co_name)
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.USE_CASE_CHOICE_REJECT, NavInsID.USE_CASE_REVIEW_TAP,
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL,
                NavInsID.WAIT_FOR_HOME_SCREEN
            ])
        elif firmware.device == "nanos":
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
            ])
        else:
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, [
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK,
                NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
            ])
    assert (backend.last_async_response.status == 0x6985)
