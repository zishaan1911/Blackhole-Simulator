# Security Policy

## Supported versions

The latest release is supported. There are no maintenance branches.

| Version | Supported |
|---------|-----------|
| 1.1.x   | yes       |
| < 1.1   | no        |

## Reporting a vulnerability

Report privately, not in a public issue:

- **Preferred:** GitHub → *Security* → *Report a vulnerability* (private
  advisory).
- **Fallback:** email **iamzishaan@gmail.com** with `SECURITY` in the subject.

Please include what you did, what happened, and the platform, driver and GPU
you saw it on. Expect an acknowledgement within 7 days and an assessment within
30. A fix ships in the next release, credited unless you ask otherwise.

## Realistic scope

This is a local desktop renderer. It opens no sockets, reads no network input
and holds no credentials. The parts worth a look:

- **`src/PngWriter.cpp`** — a hand-written PNG encoder with no external
  dependency. It only writes, but it does arithmetic on image dimensions.
- **`src/Shader.cpp`** — reads shader files from disk at startup and on `F5`,
  including a path baked in at build time.
- **The GLSL** — a malicious shader file substituted on disk could hang a GPU.
  That requires local write access to the install directory first.

Out of scope: driver crashes, GPU hangs from an unmodified shader on an
underpowered device, and anything requiring an attacker who can already write to
the installation directory.
