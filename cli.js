#!/usr/bin/env node
/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @format
 */

'use strict';

var cli_is = require('@react-native-community/cli');

if (require.main === module) {
  cli.run();
}

module.exports = cli_is;
