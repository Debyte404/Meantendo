# Contributing to Meantendo 🎮

First off, thank you for considering contributing to Meantendo! It's people like you that make Meantendo such a great open-source gaming console.

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Adding a New Game](#adding-a-new-game)

## 📜 Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

## 🚀 Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- ESP32 development board or ESP32-S3
- Basic knowledge of C++ and embedded systems

### Quick Start

1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/Meantendo.git
   cd Meantendo
   ```
3. Create a branch for your feature:
   ```bash
   git checkout -b feature/amazing-feature
   ```
4. Make your changes
5. Commit and push
6. Open a Pull Request

## 🤝 How Can I Contribute?

### 🐛 Reporting Bugs

- Use the [Bug Report template](.github/ISSUE_TEMPLATE/bug_report.md)
- Check if the issue already exists
- Include as much detail as possible
- Provide serial monitor output if applicable

### ✨ Suggesting Features

- Use the [Feature Request template](.github/ISSUE_TEMPLATE/feature_request.md)
- Explain why this feature would be useful
- Consider the hardware limitations of ESP32

### 🎮 Adding Games

We love new games! See the [Adding a New Game](#adding-a-new-game) section below.

### 📚 Improving Documentation

- Fix typos and improve clarity
- Add examples and tutorials
- Translate documentation

### 🔧 Code Contributions

- Fix bugs
- Implement new features
- Improve performance
- Refactor code

## 💻 Development Setup

### Installing Dependencies

```bash
# Install PlatformIO Core (if not using VS Code extension)
pip install platformio

# Install project dependencies
pio pkg install

# Build the project
pio run
```

### Project Structure

```
Meantendo/
├── src/
│   ├── core/          # Core system components
│   ├── games/         # Game implementations
│   ├── ota/           # OTA update system
│   └── main.cpp       # Entry point
├── include/           # Header files
├── test/              # Unit tests
├── components/        # ESP-IDF components
├── partitions/        # Partition tables
└── platformio.ini     # PlatformIO configuration
```

### Building and Flashing

```bash
# Build for default environment
pio run

# Build for specific environment
pio run -e esp32dev

# Build and upload
pio run -t upload

# Monitor serial output
pio device monitor
```

## 📐 Coding Standards

### C++ Style Guide

We follow a modified Google C++ Style Guide:

```cpp
// Use 4 spaces for indentation
// Use descriptive variable names
// Keep functions focused and small

class GameExample : public Game {
public:
    void init() override;
    void update(uint32_t deltaTime) override;
    void render(Display& display) override;
    
private:
    uint16_t score_ = 0;  // Trailing underscore for private members
    bool isRunning_ = false;
};

// Use snake_case for files: game_example.cpp
// Use PascalCase for classes: GameExample
// Use camelCase for functions/variables: updateScore()
```

### Code Quality

- Keep functions under 50 lines when possible
- Add comments for complex logic
- Use `const` where appropriate
- Avoid magic numbers - use named constants
- Handle errors gracefully

## 📝 Commit Guidelines

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Formatting, no code change
- `refactor`: Code restructuring
- `perf`: Performance improvement
- `test`: Adding tests
- `chore`: Maintenance tasks
- `game`: New game added

### Examples

```
feat(ota): add delta update support
fix(display): correct color depth for ST7789
game(snake): add classic Snake game
docs(readme): add hardware assembly guide
```

## 🔀 Pull Request Process

1. **Ensure your code builds** without errors or warnings
2. **Update documentation** if you changed functionality
3. **Add tests** if applicable
4. **Follow the PR template** when creating your PR
5. **Request review** from maintainers
6. **Address feedback** promptly
7. **Squash commits** if requested before merge

### PR Checklist

- [ ] Code compiles without warnings
- [ ] Follows coding standards
- [ ] Tests pass (if applicable)
- [ ] Documentation updated
- [ ] Commit messages follow convention
- [ ] PR description is complete

## 🎮 Adding a New Game

Want to add a new game? Awesome! Here's how:

### 1. Create the Game Class

```cpp
// src/games/MyGame.cpp

#include "games/MyGame.hpp"

void MyGame::init() {
    // Initialize game state
    score_ = 0;
    gameOver_ = false;
}

void MyGame::update(uint32_t deltaTime) {
    // Update game logic
    handleInput();
    updateGameState(deltaTime);
}

void MyGame::render(Display& display) {
    // Draw game graphics
    display.clear();
    drawScore(display);
    drawGameElements(display);
}

const char* MyGame::getName() const {
    return "My Game";
}
```

### 2. Create the Header

```cpp
// src/games/MyGame.hpp

#pragma once
#include "core/Game.hpp"

class MyGame : public Game {
public:
    void init() override;
    void update(uint32_t deltaTime) override;
    void render(Display& display) override;
    const char* getName() const override;
    
private:
    uint16_t score_ = 0;
    bool gameOver_ = false;
};
```

### 3. Register in Menu

Add your game to the game registry in the menu system.

### 4. Add Tests (Optional but Encouraged)

```cpp
// test/games/test_mygame.cpp

#include <unity.h>
#include "games/MyGame.hpp"

void test_mygame_init() {
    MyGame game;
    game.init();
    TEST_ASSERT_EQUAL(0, game.getScore());
}
```

### Game Guidelines

- Keep memory usage minimal
- Target 30+ FPS performance
- Support all input buttons
- Include a pause/menu option
- Add sound effects (if audio supported)

## 🏆 Recognition

Contributors are recognized in:

- The [README.md](README.md) contributors section
- GitHub's contributors list
- Release notes mentioning specific contributions

## ❓ Questions?

- Open a [Discussion](https://github.com/Debyte404/Meantendo/discussions)
- Check existing issues and PRs
- Read the documentation

---

Thank you for contributing to Meantendo! 🎮✨
