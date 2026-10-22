# Contributing

Welcome, and thank you for your interest in contributing to the MANA OOT
driver.

Read [`mana/README.md`](mana/README.md) for the driver design, supported
configurations, and build instructions.

## Reporting bugs

Include the distribution, architecture, kernel and driver versions,
reproduction steps, and relevant logs with sensitive information removed.
Reproduce issues on a supported distribution with a clean, non-EOL LTS kernel.

## Making changes

- Keep changes focused and update affected documentation.
- Put compatibility changes in `mana/compat/`, `configure`, or build glue,
  not the upstream snapshot in `mana/driver/`.

## Validation and submission

Build and test against all supported kernels before submission.

Follow the
[Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct).
Contributions are subject to the terms in [`COPYING`](COPYING).
