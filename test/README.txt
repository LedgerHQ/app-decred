The folder "parsed_tx" contains a few dissected transactions.
Tests are based on the mainnet ones and basically replay them.
XtoY.py are the test scripts, where X is the number of inputs consumed and Y the number of UTXO created.

These tests require the ledgerblue python3 library, and to have the Decred app opened on your test device with this BIP39 test seed:
abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about

FYI, you can input this seed in the Decrediton wallet using a hex field when setting up a wallet. the hex equivalent is : 
5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc19a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4

