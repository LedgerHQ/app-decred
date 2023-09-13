from ragger.navigator import NavInsID
from binascii import hexlify
from pathlib import Path
from inspect import currentframe


def test_addr_display(backend, firmware, navigator):
    packets = [
        "058000002c8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    path = Path(currentframe().f_code.co_name)
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_until_text_and_compare(
                NavInsID.TAPPABLE_CENTER_TAP, [
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_TAP,
                    NavInsID.WAIT_FOR_HOME_SCREEN
                ], "Show",
                Path(__file__).parent.resolve(), path)
        else:
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK, [NavInsID.BOTH_CLICK], "Approve",
                Path(__file__).parent.resolve(), path)


def test_addr_display_unusual_path(backend, firmware, navigator):
    packets = [
        "058000002b8000002a800000000000000000000001"  # BIP32 path len, BIP32 path
    ]

    packets[0] = "e0400100" + hexlify(bytes([int(len(packets[0]) / 2)
                                             ])).decode("utf-8") + packets[0]

    # 41 04589ae7c835ce76e23cf8feb32f1adf4a7f2ba0ed2ad70801802b0bcd70e99c1c2c03b4c945b672f5d9dc0e5f9cce42afb893299dbf0fce6f02e8f3de580ac5bf pub key
    # 23 5473636f46366d566741416b664e78776e716f5a553936654e3534355247594c376135 addr base58
    # c191668478d204284390538897117f8c66ef8dafd2f3e67c0d83ce4fe4f09e53  chaincode

    path = Path(currentframe().f_code.co_name)
    with backend.exchange_async_raw(data=bytearray.fromhex(packets[0])) as r:
        if firmware.device == "stax":
            navigator.navigate_and_compare(
                Path(__file__).parent.resolve(), path, [
                    NavInsID.USE_CASE_CHOICE_REJECT,
                    NavInsID.USE_CASE_REVIEW_TAP,
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_TAP,
                    NavInsID.WAIT_FOR_HOME_SCREEN
                ])
        elif firmware.device == "nanos":
            navigator.navigate_and_compare(
                Path(__file__).parent.resolve(), path, [
                    NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                    NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK,
                    NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                    NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
                ])
        else:
            navigator.navigate_and_compare(
                Path(__file__).parent.resolve(), path, [
                    NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK,
                    NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK,
                    NavInsID.RIGHT_CLICK, NavInsID.BOTH_CLICK
                ])
