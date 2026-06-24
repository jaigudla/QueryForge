# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

If you discover a security issue in QueryForge, please report it responsibly:

1. **Do not** open a public GitHub issue for exploitable vulnerabilities.
2. Email the maintainer or open a private security advisory on GitHub if enabled.
3. Include steps to reproduce, affected versions, and potential impact.

QueryForge is a local CLI benchmarking tool. It reads user-supplied CSV files and does not expose network services. The primary security surface is parsing untrusted CSV input; fuzzing and sanitizer CI help guard that path.

We aim to acknowledge reports within 7 days and provide a fix or mitigation plan within 30 days for confirmed issues.
