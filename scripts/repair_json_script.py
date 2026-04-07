#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JSON Repair Script
Uses json-repair library to fix malformed JSON from AI responses

Install: pip install json-repair
"""

import sys
import json

try:
    from json_repair import repair_json
except ImportError:
    print(json.dumps({"error": "json-repair not installed. Run: pip install json-repair"}))
    sys.exit(1)

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"error": "No input provided"}))
        sys.exit(1)
    
    input_json = sys.argv[1]
    
    try:
        repaired = repair_json(input_json)
        print(repaired)
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        sys.exit(1)

if __name__ == "__main__":
    main()
