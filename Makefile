# ****************************************************************************
#    Decred Ledger App
#    (c) 2023 Ledger SAS.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
# ****************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.defines

########################################
#        Mandatory configuration       #
########################################

# Enabling DEBUG flag will enable PRINTF and disable optimizations
#DEBUG = 1

# Application version
APPVERSION_M= 1
APPVERSION_N= 3
APPVERSION_P= 14
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src

# Application icons
ICON_NANOS = nanos_app_$(COIN).gif
ICON_STAX = stax_app_$(COIN).gif
ICON_NANOX = nanox_app_$(COIN).gif
ICON_NANOSP = nanox_app_$(COIN).gif

# Application allowed derivation curves.
CURVE_APP_LOAD_PARAMS = secp256k1

# Application allowed derivation paths.
PATH_APP_LOAD_PARAMS = "44'/42'"

# Setting to allow building variant applications
VARIANT_PARAM = COIN
VARIANT_VALUES = decred decred_testnet

ifndef COIN
COIN=decred
endif

########################################
#     Application custom permissions   #
########################################
# See SDK `include/appflags.h` for the purpose of each permission
HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
HAVE_APPLICATION_FLAG_DERIVE_MASTER = 1
HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1

ifeq ($(COIN),decred)
# Decred mainnet
DEFINES_LIB = # Decred IS the lib
DEFINES += COIN_P2PKH_VERSION=1855 COIN_P2SH_VERSION=1818 COIN_FAMILY=1 COIN_COINID=\"Decred\" COIN_COINID_HEADER=\"DECRED\" COIN_COLOR_HDR=0x5482ff COIN_COLOR_DB=0xB2E8CB COIN_COINID_NAME=\"Decred\" COIN_COINID_SHORT=\"DCR\" COIN_KIND=COIN_KIND_DECRED
APPNAME = "Decred"
HAVE_APPLICATION_FLAG_LIBRARY = 1
else ifeq ($(COIN),decred_testnet)
# Decred testnet
# All but Decred app use dependency onto the decred app/lib
DEFINES_LIB = USE_LIB_DECRED
DEFINES += COIN_P2PKH_VERSION=3873 COIN_P2SH_VERSION=3836 COIN_FAMILY=1 COIN_COINID=\"Decred\" COIN_COINID_HEADER=\"DECRED\" COIN_COLOR_HDR=0x5482ff COIN_COLOR_DB=0xB2E8CB COIN_COINID_NAME=\"Decred\" COIN_COINID_SHORT=\"TDCR\" COIN_KIND=COIN_KIND_DECRED_TESTNET
APPNAME = "Decred Test"
else
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error Unsupported COIN - use decred, decred_testnet)
endif
endif

DEFINES += $(DEFINES_LIB) TCS_LOADER_PATCH_VERSION=0

# Remove warning on custom snprintf implementation usage
CFLAGS    += -Wno-format

# U2F
DEFINES   += HAVE_IO_U2F U2F_PROXY_MAGIC=\"BTC\"
SDK_SOURCE_PATH  += lib_stusb lib_stusb_impl lib_u2f 

########################################
# Application communication interfaces #
########################################
ENABLE_BLUETOOTH = 1

########################################
#         NBGL custom features         #
########################################
ENABLE_NBGL_QRCODE = 1

# Use only specific files from standard app
DISABLE_STANDARD_APP_FILES = 1
APP_SOURCE_FILES += ${BOLOS_SDK}/lib_standard_app/io.c
INCLUDES_PATH += ${BOLOS_SDK}/lib_standard_app

include $(BOLOS_SDK)/Makefile.standard_app