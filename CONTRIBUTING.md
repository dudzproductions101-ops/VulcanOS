# Contributing to VulcanOS

Thank you for your interest in contributing to VulcanOS!

Every contribution helps improve the operating system, whether it is code, documentation, testing, bug reports, or community support. VulcanOS grows through its contributors.

---

## 1. Code of Conduct

Please follow our [Code of Conduct](CODE_OF_CONDUCT.md).

Be respectful, constructive, and keep discussions focused on improving VulcanOS.

---

## 2. Getting Started

Before contributing:

- Check the issue tracker for `good first issue` or `help wanted` tasks.
- Discuss large changes before implementation.
- Open an RFC for major kernel, driver, architecture, or API changes.

---

## 3. Development Workflow

Create a branch from `main`:

    git switch -c feature/my-change

Before submitting:

- Build VulcanOS successfully.
- Test your changes.
- Keep commits focused and readable.

Use Conventional Commits:

    feat: add new feature
    fix: fix a bug
    docs: update documentation
    refactor: improve code structure
    ci: update build systems
    test: add tests

---

## 4. Commit Requirements

VulcanOS uses DCO sign-offs.

Commit with:

    git commit -s -m "your message"

Commit signing with SSH or GPG is recommended:

    git commit -S -s -m "your message"

---

## 5. AI Usage Policy

AI tools may be used for learning, documentation, or assistance.

AI-generated code is discouraged. If AI is used:

- Mention it in your pull request.
- Review all generated code.
- Test everything carefully.

Contributors are responsible for submitted code.

---

## 6. Coding Standards

Keep VulcanOS reliable:

- Check pointers and allocations.
- Avoid memory leaks.
- Document complex kernel or hardware code.
- Write clear and maintainable code.

---

## 7. Pull Requests

When submitting a PR:

- Explain what changed and why.
- Describe testing performed.
- Ensure CI passes.
- Respond constructively to reviews.

Thank you for helping build VulcanOS!
