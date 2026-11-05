#!/usr/bin/env python3
"""Scaffold a lab face project: python3 newface.py <dir-name> "<Display Name>" """
import json, os, sys, uuid, shutil
name, display = sys.argv[1], sys.argv[2]
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", name)
os.makedirs(os.path.join(root, "src", "c"), exist_ok=True)
pkg = {"name": name, "author": "edutch95", "version": "1.0.0", "keywords": ["pebble-app"], "private": True,
       "dependencies": {},
       "pebble": {"displayName": display, "uuid": str(uuid.uuid4()), "projectType": "native", "sdkVersion": "3",
                  "enableMultiJS": False, "targetPlatforms": ["gabbro"], "watchapp": {"watchface": True},
                  "capabilities": ["health"], "resources": {"media": []}}}
json.dump(pkg, open(os.path.join(root, "package.json"), "w"), indent=2)
open(os.path.join(root, "package.json"), "a").write("\n")
shutil.copy(os.path.join(os.path.dirname(__file__), "..", "..", "desk-pet", "wscript"), os.path.join(root, "wscript"))
open(os.path.join(root, ".gitignore"), "w").write("build/\n.lock-waf*\n__pycache__/\n")
os.makedirs(os.path.join(root, "docs"), exist_ok=True)
print("created", root)
