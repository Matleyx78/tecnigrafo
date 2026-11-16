#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
cd ~/progetti/solid-space && git checkout main && git pull
cd ~/progetti/tecnigrafo && git checkout main && git pull   && git submodule update --init third_party/solid-space
python3 scripts/dipendenze.py 0.8
