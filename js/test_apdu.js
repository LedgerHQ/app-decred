#!/usr/bin/env node
/********************************************************************************
*   Ledger Communication toolkit
*   (c) 2016-2019 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

// Byte-level checks of the HASH_SIGN (INS 0x48) APDU built by the JS client,
// against the known-good Decred vector used by test/test_1to2.py.
// Run with: node js/test_apdu.js

'use strict';

var assert = require('assert');

function buildSignApdu(module, path, lockTime, sigHashType, expiry) {
	var LedgerBtc = require(module);
	var btc = Object.create(LedgerBtc.prototype);
	var captured = null;
	btc.comm = {
		exchange: function (hex) {
			captured = hex;
			return Promise.resolve('30'.padEnd(142, '0') + '9000');
		}
	};
	return btc.signTransaction_async(path, lockTime, sigHashType, expiry).then(function () {
		return captured;
	});
}

var modules = ['./ledger-btc.js', './dist/ledger-btc.js'];

// path len + path, locktime, expiry, sighash - no obsolete authorization byte,
// Lc is 0x1e for a five-component path.
var EXPECTED_ZERO =
	'e04800001e058000002c8000002a800000000000000000000001000000000000000001';

Promise.all(modules.map(function (m) {
	return buildSignApdu(m, "44'/42'/0'/0/1", 0, 1, 0).then(function (apdu) {
		assert.strictEqual(apdu, EXPECTED_ZERO, m + ': APDU mismatch');
		console.log(m + ': known-good vector OK');
	});
})).then(function () {
	// Zero values hide byte-order bugs: the app reads these fields big-endian.
	return Promise.all(modules.map(function (m) {
		return buildSignApdu(m, "44'/42'/0'/0/1", 0x01020304, 1, 0x11223344).then(function (apdu) {
			var fields = apdu.slice(12 + 40, 12 + 40 + 16);
			assert.strictEqual(fields, '0102030411223344', m + ': locktime/expiry not big-endian');
			console.log(m + ': big-endian locktime/expiry OK');
		});
	}));
}).then(function () {
	console.log('all checks passed');
}).catch(function (error) {
	console.error(error.message || error);
	process.exit(1);
});
