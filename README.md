# PhantomPong

PhantomPong is a simple yet polished clone of the classic Pong game, built using [raylib](https://www.raylib.com/). It features smooth paddle controls, score tracking, sound effects, and a stylish retro-futuristic aesthetic.

---

## 🎮 Features

- ⚙️ Built in C with raylib
- 🔊 Sound effects and background music
- 🖋️ Custom fonts
- 📦 Minimal external dependencies
- 🎯 Cross-platform potential (Windows/Linux)

---

## 🛠️ Requirements

- [raylib](https://www.raylib.com/) (included statically via `lib/` and `include/`)
- A C compiler (GCC, Clang, or MSVC)

---

## 🚀 How to Run

### On Windows:

Just double-click the `run.bat` file (if it exists), or compile manually:

```bash
gcc main.c -o Pong.exe -Iinclude -Llib -lraylib -lopengl32 -lgdi32 -lwinmm
./Pong.exe
```

---

## 📁 Project Structure

```
PhantomPong/
├── assets/         # Audio and font resources
├── include/        # raylib headers
├── lib/            # Static raylib library
├── main.c          # Game source code
├── .gitignore
└── README.md
```

---

## 📜 License

This project is open source under the [BSD 2-Clause License](LICENSE). Raylib itself uses the zlib/libpng license.

---

## 🙌 Credits

- **Audio:** [Onyx - Ataraxia](#)
- **Fonts:** Toxigenesis, Exo 2
- **Game Framework:** [raylib](https://www.raylib.com/)

---

Enjoy the retro vibes! 🚀