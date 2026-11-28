# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/ci-errori.sh, without network: gh and curl are replaced by
# fakes that answer with fixed JSON and write down how they were called. They
# check which credential reads the error annotations (docs/agenti/catena.md):
# the job token of GH_TOKEN_LETTURA where it is set, otherwise none.
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent / "ci-errori.sh"

# Answers by the end of the API path. The fakes log one line per call:
# "<tool> <credential> <path>", where the credential is the GH_TOKEN that gh
# sees, or the Authorization header curl was given ("-" when there is none).
FAKE = r'''#!/usr/bin/env bash
tool=${0##*/}
if [ "$tool" = gh ]; then
    path=$2
    cred=${GH_TOKEN:--}
else
    path=${*: -1}
    path=${path#https://api.github.com/}
    cred=-
    prev=
    for a in "$@"; do
        if [ "$prev" = -H ] && [[ $a == Authorization:* ]]; then cred=${a#Authorization: Bearer }; fi
        prev=$a
    done
fi
echo "$tool $cred $path" >> "$FAKE_LOG"
case $path in
    */annotations*) echo '[{"annotation_level":"failure","path":"core/a.cpp","start_line":3,"title":"","message":"boom"}]' ;;
    */pulls/*) echo '{"head":{"sha":"abc1234"}}' ;;
    *runs\?head_sha=*) echo '{"workflow_runs":[{"id":4279201937}]}' ;;
    */jobs*) echo '{"jobs":[{"id":5,"name":"Linux","conclusion":"failure","steps":[{"name":"Build","conclusion":"failure"}]}]}' ;;
    */runs/*) echo '{"id":7,"status":"completed","conclusion":"failure","html_url":"https://example.invalid/7"}' ;;
    *) echo '{}' ;;
esac
'''


@unittest.skipUnless(shutil.which("jq") and shutil.which("bash"), "needs bash and jq")
class Annotations(unittest.TestCase):
    def run_script(self, env_extra: dict[str, str], tools=("gh", "curl"),
                   arg: str = "4279201937") -> tuple[str, list[str]]:
        with tempfile.TemporaryDirectory() as tmp:
            bin_dir = Path(tmp) / "bin"
            bin_dir.mkdir()
            for tool in tools:
                fake = bin_dir / tool
                fake.write_text(FAKE)
                fake.chmod(0o755)
            log = Path(tmp) / "log"
            log.touch()
            # Only the fakes, bash and jq: no real gh or curl can be reached.
            for tool in ("jq", "bash"):
                (bin_dir / tool).symlink_to(shutil.which(tool))
            env = {"PATH": str(bin_dir), "FAKE_LOG": str(log), "HOME": tmp}
            env.update(env_extra)
            out = subprocess.run(["bash", str(SCRIPT), arg], env=env, capture_output=True,
                                 text=True, check=True).stdout
            return out, log.read_text().splitlines()

    def annotation_calls(self, calls: list[str]) -> list[str]:
        return [c for c in calls if "/annotations" in c]

    def test_job_token_in_github_actions(self):
        out, calls = self.run_script({"GH_TOKEN": "personale", "GH_TOKEN_LETTURA": "lettura"})
        self.assertIn("core/a.cpp:3 boom", out)
        self.assertEqual([c.split()[:2] for c in self.annotation_calls(calls)], [["gh", "lettura"]])
        # The other calls keep the personal token.
        self.assertTrue(all(c.split()[1] == "personale" for c in calls if "/annotations" not in c))

    def test_job_token_without_gh(self):
        out, calls = self.run_script({"GH_TOKEN_LETTURA": "lettura"}, tools=("curl",))
        self.assertIn("core/a.cpp:3 boom", out)
        self.assertEqual([c.split()[:2] for c in self.annotation_calls(calls)], [["curl", "lettura"]])

    def test_no_authentication_elsewhere(self):
        out, calls = self.run_script({"GH_TOKEN": "personale", "GITHUB_TOKEN": "altro"})
        self.assertIn("core/a.cpp:3 boom", out)
        self.assertEqual([c.split()[:2] for c in self.annotation_calls(calls)], [["curl", "-"]])


    def test_pull_request_number(self):
        # A short number is a pull request: the run of its last commit.
        out, calls = self.run_script({}, arg="773")
        self.assertIn("pull request #773: commit abc1234", out)
        self.assertTrue(any(c.endswith("/pulls/773") for c in calls))
        self.assertTrue(any("runs?head_sha=abc1234" in c for c in calls))

    def test_run_id(self):
        out, calls = self.run_script({}, arg="4279201937")
        self.assertFalse(any("/pulls/" in c for c in calls))
        self.assertTrue(any(c.endswith("/runs/4279201937") for c in calls))


# Three workflows on one commit: the CI (red, older), Submodule pointer
# (green, newest) and the sentinel (red). Run ids tell them apart.
FAKE_WORKFLOWS = r"""#!/usr/bin/env bash
path=$2
echo "gh - $path" >> "$FAKE_LOG"
case $path in
    */annotations*) echo '[{"annotation_level":"failure","path":"x","start_line":1,"title":"","message":"boom"}]' ;;
    *runs\?head_sha=*) echo '{"workflow_runs":[
        {"id":30,"path":".github/workflows/puntatore.yml","conclusion":"success"},
        {"id":20,"path":".github/workflows/sentinella.yml","conclusion":"failure"},
        {"id":11,"path":".github/workflows/ci.yml","conclusion":"failure"},
        {"id":10,"path":".github/workflows/ci.yml","conclusion":"success"}]}' ;;
    */jobs*) echo '{"jobs":[{"id":5,"name":"Linux","conclusion":"failure","steps":[{"name":"Build","conclusion":"failure"}]}]}' ;;
    */runs/*) id=${path##*/}; echo "{\"id\":$id,\"status\":\"completed\",\"conclusion\":\"failure\",\"html_url\":\"u\"}" ;;
    *) echo '{}' ;;
esac
"""


@unittest.skipUnless(shutil.which("jq") and shutil.which("bash"), "needs bash and jq")
class WhichRun(unittest.TestCase):
    def test_commit_reads_the_ci_and_the_other_red_runs(self):
        with tempfile.TemporaryDirectory() as tmp:
            bin_dir = Path(tmp) / "bin"
            bin_dir.mkdir()
            fake = bin_dir / "gh"
            fake.write_text(FAKE_WORKFLOWS)
            fake.chmod(0o755)
            for tool in ("jq", "bash"):
                (bin_dir / tool).symlink_to(shutil.which(tool))
            log = Path(tmp) / "log"
            log.touch()
            env = {"PATH": str(bin_dir), "FAKE_LOG": str(log), "HOME": tmp}
            out = subprocess.run(["bash", str(SCRIPT), "abc1234"], env=env, capture_output=True,
                                 text=True, check=True).stdout
        runs = [line.split(":")[0] for line in out.splitlines() if line.startswith("run ")]
        # The newest CI run first, then the red sentinel; not the green pointer
        # nor the older CI run.
        self.assertEqual(runs, ["run 11", "run 20"])


if __name__ == "__main__":
    unittest.main()
