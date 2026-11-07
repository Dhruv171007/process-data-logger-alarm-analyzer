# Project Proposal: Process Data Logger and Alarm Analyzer

# 1. Project Information

## 1.1 Project Title

Process Data Logger and Alarm Analyzer for Industrial Process Monitoring

## 1.2 Engineering Domain

Manufacturing (industrial process instrumentation)

## 1.3 Student Details

| Field                  | Details                                                                  |
|------------------------|--------------------------------------------------------------------------|
| Student name           | Dhruv Ishwar Patil                                                       |
| Roll number            | 612509017                                                                |
| Division / batch       | Instrumentation and Control                                              |
| Repository name        | process-data-logger-alarm-analyzer                                       |
| GitHub repository link | https://github.com/Dhruv171007/process-data-logger-alarm-analyzer        |

## 1.4 Individual Project Declaration

I confirm that this project is my individual work. I understand the design, source code, unit tests, Makefile and documentation included in this repository, and I am prepared to explain or modify any part during evaluation.

Student acknowledgement: Dhruv Ishwar Patil

---

# 2. Engineering Scenario

## 2.1 Scenario Description

In a manufacturing plant, process variables such as temperature, pressure and flow are measured by field instruments (transmitters) and recorded at regular intervals. Each measurement point is identified by a tag (for example `TT-101` for a temperature transmitter). Every tag has alarm limits that define when a reading is unsafe or abnormal.

- **Where it occurs:** Process plants, boiler rooms, utility sections and laboratory pilot plants.
- **Who is involved:** Instrumentation technicians, control room operators and shift engineers.
- **Current working method:** Readings are noted manually or exported as raw log files and checked by eye or in spreadsheets.
- **Information to be managed:** Tag details (ID, description, unit, alarm limits) and time-stamped readings.
- **Problems in the current method:** Alarm violations are missed or found late, statistics are calculated by hand, and shift reports take time and contain errors.

## 2.2 Problem Statement

In industrial process plants, operators and instrumentation engineers currently face difficulty in reviewing large sets of logged process readings against alarm limits. The current method results in missed alarm events, calculation errors and slow shift reporting. The proposed software will provide tag management, validated reading entry and import, alarm classification, statistical analysis and automatic report generation so that users can quickly identify abnormal process behaviour and produce accurate shift reports.

## 2.3 Proposed Solution

The proposed solution is a modular, menu-driven command-line application written in C.

- **What it does:** Manages measurement tags and their alarm limits, accepts process readings by manual entry or CSV import, classifies every reading against its tag limits, and analyses the data.
- **How it addresses the problem:** Validation rejects bad data, classification finds alarm conditions automatically, and statistics and reports remove manual calculation.
- **Information managed:** Tags (ID, description, unit, four alarm limits) and readings (tag ID, timestamp, value).
- **Calculations and decisions:** Minimum, maximum, mean, standard deviation, alarm status (Normal, Low, Low-Low, High, High-High) and time spent in alarm.
- **Outputs:** On-screen tables of filtered and sorted readings, per-tag statistics, and a shift report saved as a text file.
- **Storage:** Tags and readings are saved to files and reloaded when the application starts.

## 2.4 Target Users

| User Type          | Responsibility                                 | How the User Will Use the Application                                              |
|--------------------|------------------------------------------------|------------------------------------------------------------------------------------|
| Control room operator | Monitors process variables during a shift   | Imports or enters readings, views alarm summaries and filters readings by status   |
| Instrumentation technician | Maintains field instruments and tag data | Adds, edits and deletes tags and sets alarm limits                                 |
| Shift engineer     | Reviews performance and reports                | Views statistics per tag and generates the shift report                            |

---

# 3. Project Scope

## 3.1 In-Scope Features

1. Add, edit, delete, search and list measurement tags with alarm limits
2. Add process readings manually with full input validation
3. Import readings from a CSV file, skipping and counting invalid lines
4. Classify readings into alarm states using the tag limits
5. Filter readings by tag, alarm status and time range
6. Sort readings by value or timestamp
7. Calculate minimum, maximum, mean and standard deviation per tag
8. Calculate time spent in alarm per tag
9. Generate a shift report as a text file
10. Save and load tags and readings from files

## 3.2 Out-of-Scope Features

1. Real-time data acquisition from hardware or PLCs
2. Graphical user interface or trend plotting
3. Multi-user access, login and networking
4. Database storage
5. Rate-of-change and predictive analysis

## 3.3 Assumptions

1. Readings for a tag are logged at a fixed sampling interval, defined as a constant (60 seconds by default).
2. Timestamps use the format `YYYY-MM-DD HH:MM:SS`.
3. Each tag has one fixed engineering unit.
4. The application is used by a single user at a time.
5. CSV files follow the format `tag_id,timestamp,value`.

## 3.4 Constraints

1. The application is written in C; only the test files use C++ (for Google Test).
2. Command-line interface only.
3. File-based storage with fixed maximum records (`MAX_TAGS`, `MAX_READINGS`).
4. Individual project with limited development time.
5. Must compile without warnings using `-Wall -Wextra -Wpedantic`.

---

# 4. Requirements

## 4.1 Functional Requirements

| ID    | Functional Requirement                                                                       | Priority | Verification Method |
|-------|----------------------------------------------------------------------------------------------|----------|---------------------|
| FR-01 | The system shall add a new tag with ID, description, unit and four alarm limits.             | High     | Demonstration / Test |
| FR-02 | The system shall validate tag IDs for format and reject duplicates.                          | High     | Test                |
| FR-03 | The system shall edit the description, unit and alarm limits of an existing tag.             | Medium   | Demonstration       |
| FR-04 | The system shall delete a tag together with its readings.                                    | Medium   | Demonstration / Test |
| FR-05 | The system shall search for a tag by ID and list all tags.                                   | High     | Demonstration / Test |
| FR-06 | The system shall add a reading manually after validating tag, timestamp and value.           | High     | Demonstration / Test |
| FR-07 | The system shall import readings from a CSV file and report how many lines were rejected.    | High     | Demonstration       |
| FR-08 | The system shall reject readings with an unknown tag, invalid timestamp or invalid value.    | High     | Test                |
| FR-09 | The system shall classify each reading as Normal, Low, Low-Low, High or High-High.           | High     | Test                |
| FR-10 | The system shall filter readings by tag, alarm status and time range.                        | Medium   | Demonstration / Test |
| FR-11 | The system shall sort readings by value or by timestamp.                                     | Medium   | Test                |
| FR-12 | The system shall calculate minimum, maximum, mean and standard deviation for a tag.          | High     | Test                |
| FR-13 | The system shall calculate the time a tag spent in alarm.                                    | Medium   | Test                |
| FR-14 | The system shall generate a shift report and save it to a text file.                         | High     | Demonstration       |
| FR-15 | The system shall save tags and readings to files and load them at startup.                   | High     | Demonstration       |
| FR-16 | The system shall handle invalid menu choices and missing or corrupt files without crashing.  | High     | Negative testing    |

## 4.2 Non-Functional Requirements

| ID     | Non-Functional Requirement                                                   | Verification Method |
|--------|------------------------------------------------------------------------------|---------------------|
| NFR-01 | The source code shall compile without warnings.                              | Build output        |
| NFR-02 | The project shall use a modular multi-file structure.                        | Code review         |
| NFR-03 | The application shall handle invalid input without crashing.                 | Negative testing    |
| NFR-04 | Important functions shall be tested using gTest.                             | Unit-test output    |
| NFR-05 | The project shall build using a Makefile.                                    | Build demonstration |
| NFR-06 | The project shall be maintained in a GitHub repository.                      | Repository review   |
| NFR-07 | The code shall use named constants instead of magic numbers.                 | Code review         |
| NFR-08 | The code shall use safe input functions (`fgets`) and check all file operations. | Code review     |

## 4.3 Requirement Traceability

This table will be completed after the detailed design and unit-test plan are prepared.

| Requirement ID | Module         | Function or Feature       | Test ID |
|----------------|----------------|---------------------------|---------|
| FR-01          | tag            | addTag()                  | To be assigned |
| FR-02          | validation     | validateTagId()           | To be assigned |
| FR-09          | alarm          | classifyReading()         | To be assigned |
| FR-12          | statistics     | calculateMean(), calculateStdDev() | To be assigned |

---

# 5. High-Level Architecture

## 5.1 Architecture Description

The software is organised in layers. `main.c` controls the menu and workflow only. Each module has one responsibility and communicates through its header file.

- **User interface layer:** `main.c` (menu and workflow control)
- **Data-management layer:** `tag` and `reading` modules (store and retrieve records)
- **Validation layer:** `validation` module (checks all user and file input)
- **Processing layer:** `statistics` and `alarm` modules (calculations and decisions)
- **File-storage layer:** `file_handler` module (load, save and CSV import)
- **Report layer:** `report` module (screen output and shift report file)
- **Shared definitions:** `common.h` (status codes and constants)

## 5.2 Architecture Diagram

```
                         +------------------+
                         |       User       |
                         +--------+---------+
                                  |
                         +--------v---------+
                         |  main.c (Menu)   |
                         +--------+---------+
                                  |
    +-----------+-----------+-----+-----+------------+-----------+
    |           |           |           |            |           |
+---v---+  +----v----+ +----v-----+ +---v----+ +-----v-----+ +---v----+
|  tag  |  | reading | |validation| | alarm  | | statistics| | report |
+---+---+  +----+----+ +----------+ +---+----+ +-----+-----+ +---+----+
    |           |                       |            |           |
    +-----------+-----------+-----------+------------+           |
                            |                                    |
                    +-------v--------+                           |
                    |  file_handler  |                           |
                    +-------+--------+                           |
                            |                                    |
              +-------------v-------------+          +-----------v----------+
              | tags file / readings file |          | Shift report (.txt)  |
              | CSV import file           |          +----------------------+
              +---------------------------+
```

A cleaner version of this diagram will be saved as `docs/architecture.png`.

## 5.3 High-Level Workflow

1. Application starts and initialises its data arrays.
2. Tags and readings are loaded from the data files (a missing file is handled as a first run).
3. The main menu is displayed:
   1. Manage tags
   2. Add reading
   3. Import readings from CSV
   4. View, filter and sort readings
   5. Show statistics
   6. Show alarm summary
   7. Generate shift report
   8. Save data
   0. Save and exit
4. The user enters a choice, which is validated.
5. The matching module is called.
6. A success or error message is shown.
7. The menu is shown again until the user exits.
8. Data is saved and the application ends.

---

# 6. Module Design (Initial)

| Module Name      | Source File     | Header File       | Responsibility                                                    |
|------------------|-----------------|-------------------|-------------------------------------------------------------------|
| Main application | main.c          | Not applicable    | Menu display and high-level workflow control                      |
| Common           | Not applicable  | common.h          | Status codes, constants and shared definitions                    |
| Tag              | tag.c           | tag.h             | Add, edit, delete, search and list tags                           |
| Reading          | reading.c       | reading.h         | Store readings, filter and sort them                              |
| Validation       | validation.c    | validation.h      | Validate IDs, timestamps, numeric values and menu input           |
| Statistics       | statistics.c    | statistics.h      | Minimum, maximum, mean and standard deviation                     |
| Alarm            | alarm.c         | alarm.h           | Classify readings and calculate time in alarm                     |
| File handler     | file_handler.c  | file_handler.h    | Save and load tags and readings, import CSV, handle file errors   |
| Report           | report.c        | report.h          | Display tables and write the shift report                         |

---

# 7. Planned C Concepts

| C Concept                    | Why It Is Needed                                                        |
|------------------------------|-------------------------------------------------------------------------|
| Functions                    | Give each operation one clear responsibility                            |
| Structures (`Tag_t`, `Reading_t`) | Group related data for tags and readings                           |
| Enumerations                 | Represent alarm states, sort options and status codes                   |
| Arrays                       | Store tags and readings with fixed maximum sizes                        |
| Strings                      | Handle tag IDs, descriptions and timestamps                             |
| Pointers                     | Pass arrays and structures to functions and return results via parameters |
| Constants (`#define`)        | Avoid magic numbers for sizes, limits and the sampling interval         |
| File handling                | Persist data, import CSV files and write reports                        |
| Status codes                 | Report success and errors consistently from every function              |
| Header guards and `extern "C"` | Allow modular headers that work with the C++ gTest files              |

---

# 8. Planned Repository Structure

```
process-data-logger-alarm-analyzer/
|-- src/        main.c, tag.c, reading.c, statistics.c, alarm.c,
|               validation.c, file_handler.c, report.c
|-- include/    common.h and one header per module
|-- tests/      test_statistics.cpp, test_alarm.cpp, test_validation.cpp
|-- data/       sample_readings.csv, sample_tags.txt
|-- docs/       project_proposal.md, detailed_design.md, architecture.png
|-- Makefile
|-- README.md
|-- .gitignore
`-- LICENSE
```
