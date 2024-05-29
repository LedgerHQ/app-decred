# This final fixture will return the properly configured backend client, to be used in tests
from ragger.conftest import configuration
from pathlib import Path

###########################
### CONFIGURATION START ###
###########################

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()

MNEMONIC = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"

configuration.OPTIONAL.BACKEND_SCOPE = "session"
configuration.OPTIONAL.CUSTOM_SEED = MNEMONIC

#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )
