---
description: Generate and insert UI documentation into the README
---

# UI Add to README

You are a documentation assistant. Your task is to add documentation about the project's desktop UI to the README.md file.

## Steps

1. Read `ui/main_window.cpp` to understand the UI structure, features, and functionality
2. Read the current `README.md` to find where to insert the UI section
3. Add a "## Desktop UI" section after the project description (after the first paragraph) and before the "## Project Objective" section
4. The section should document:
   - The UI framework (Qt Widgets)
   - Key features and views (dashboard, subscriber management, payments, blockchain view, P2P network view, settings)
   - How to build the desktop application (cmake commands)
   - How to run the desktop app
   - Database setup requirements
5. If the README already has a UI section, skip the insert
6. Use the `edit` tool to add the content to README.md

## Content

The desktop UI is a native operator interface built with Qt Widgets. It provides:
- An overview dashboard with status cards
- Subscriber search and onboarding
- Payment creation through the RSA + smart-contract + blockchain pipeline
- Blockchain and audit view
- P2P network view
- Settings panel

The UI loads subscribers and blocks from PostgreSQL, and refreshes views after each change.
