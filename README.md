# Book Club App

A desktop client-server application for managing a book club / bookstore platform, built with **Qt (C++/QML)** and **SQLite**. The system supports readers, publishers, and administrators through dedicated panels, with a standalone server handling the database and business logic over the network.

## Features

- **Authentication** — login, signup, and password recovery flows (`src/auth`)
- **User panel** — home page with browsable genre rows, book details, shopping cart, wishlist, a "My Library" shelf view with reading progress, and a built-in PDF reader (`src/userPanel`)
- **Publisher panel** — add/edit books, set special offers, manage publisher profile (`src/publisherPanel`)
- **Admin panel** — manage books, users, publishers, and reviews from tabbed views (`src/adminPanel`)
- **Social features** — reviews and other social interactions backed by the database layer
- **Charts** — data visualization via Qt Charts
- **Client-server architecture** — the client communicates with a standalone server backend over the network, with the server owning all database access

> **Branch note:** The User Panel above is developed on the [`userPanel-feature`](https://github.com/saleh006/Book-Club-_-Ap-Project/tree/userPanel-feature) branch and isn't merged into `main` yet. Check out that branch to build/run it. Other in-progress work also lives in its own feature branch (`auth-feature`, `database-feature`, `publisherPanel-feature`, `adminPanel-feature`, `serverClient-feature`).

## Tech Stack

- **Language:** C++ and QML
- **Framework:** Qt 6 (or Qt 5) — Widgets, Quick, QuickWidgets, Sql, Network, Charts
- **Database:** SQLite (via `QSqlDatabase`)
- **Build system:** CMake (3.19+)

## Project Structure

```
.
├── main_client.cpp          # Client application entry point
├── main_server.cpp          # Server application entry point
├── src/
│   ├── auth/                # Login, signup, password recovery (QML + C++)
│   ├── database/            # DatabaseManager split by domain (user, book, cart,
│   │                         library, social, publisher)
│   ├── userPanel/           # Reader-facing UI: browsing, cart, wishlist, My Library,
│   │                         PDF reader (on the `userPanel-feature` branch)
│   ├── publisherPanel/      # Publisher-facing book & offer management UI
│   ├── adminPanel/          # Admin dashboard: books, users, publishers, reviews
│   ├── server/              # Server manager/UI (server side) and server window (client side)
│   └── shared/              # Shared data models used by both client and server
├── res/                     # Qt resources (icons, images, .qrc file)
├── images/                  # Image assets
├── UML/                     # UML diagrams for the project design
└── CMakeLists.txt
```

## Architecture

The project builds two separate executables from a single CMake project:

| Target | Description |
|---|---|
| `BookClubServer` | Owns the SQLite database and exposes server-side logic (`ServerManager`, `ServerUI`) |
| `BookClubClient` | The user-facing desktop app, combining Qt Widgets and a QML-based auth module (`BookClubAuth`) |

The client and server communicate over the network; the server is the single source of truth for persisted data.

## Getting Started

### Prerequisites

- [Qt 6](https://www.qt.io/download) (or Qt 5) with the following modules: `Widgets`, `Sql`, `Network`, `QuickWidgets`, `Quick`, `Charts`
  - On the `userPanel-feature` branch, also add `Pdf`/`PdfWidgets` (needed for the in-app PDF reader)
- CMake 3.19 or later
- A C++ compiler with Qt's supported toolchain (MSVC on Windows, GCC/Clang on Linux/macOS)

### Build

```bash
git clone https://github.com/saleh006/Book-Club-_-Ap-Project.git
cd Book-Club-_-Ap-Project
cmake -B build -S .
cmake --build build
```

This produces two executables: `BookClubServer` and `BookClubClient`.

### Run

Start the server first, then launch the client:

```bash
./build/BookClubServer
./build/BookClubClient
```

> On first run, the server initializes the SQLite database used for users, books, carts, libraries, and reviews.

## Roadmap / Notes

- UML diagrams describing the design are available in the `UML/` folder for reference.

## License

No license has been specified yet for this project. Consider adding one (e.g., MIT) if you plan to share or accept contributions.
