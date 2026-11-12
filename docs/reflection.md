# Reflection

*Draft written from what actually happened during this project. Rewrite it in your own words and keep only what is true for you.*

## What Worked Well

- **Designing before coding.** Writing the proposal and the detailed design first (data structures, function specifications, validation, file and error handling) meant that coding was one module at a time, and the design could be checked against the code at the end.
- **One return type for errors.** Using the `Status_t` enumeration in every function kept error handling consistent. Callers never guess what went wrong, and the tests can check the exact status.
- **Validation in one module.** Typed input and file data go through the same rules, so a rule is written and tested once.
- **Small, tested modules.** Each module was built and tested before the next one started. The `statistics`, `alarm` and `validation` modules have no dependencies on other feature modules, so they were easy to test on their own.
- **Frequent commits.** Committing after each module made the history show the gradual development and made mistakes easy to find.

## Challenges and How They Were Resolved

| Challenge | How it was resolved |
|-----------|---------------------|
| Installing the compiler tools: MSYS2 downloads failed with a 503 error from the mirror server | The error was temporary. Retrying the install (and switching mirror if needed) fixed it. I also learned that the tools must be used from the **MSYS2 UCRT64** terminal, not the plain MSYS one. |
| Linker error "multiple definition" when running `make test` | The test file for validation contained a copy of the C source code instead of the tests, so every function was defined twice. Checking the first lines of the file showed this, and replacing the file with the real tests fixed it. I learned that a test file should only call the functions, and that checking the first lines of a copied file avoids this. |
| `make test` ran 0 tests | The test files were not in the `tests/` folder the Makefile searches. Listing the folder with `ls tests` showed the problem. |
| Git: the history showed an unexpected commit message next to a file | `git log` and `git show --stat` showed what each commit contained. I learned that GitHub shows the latest commit that touched each file. |
| Git: `git restore` discarded edits that I had not checked first | I learned to run `git diff` before `git restore`, and to use the VS Code Timeline view to recover old versions. |
| VS Code showed "#include errors detected" although the build worked | The editor did not know about the `include/` folder. Adding an include path in the C/C++ settings fixed it. |

## What I Learned

- How to split a program into modules with header files, `extern "C"` guards and one clear responsibility each
- How to write unit tests with Google Test, including positive, negative and boundary cases, test fixtures, temporary files and capturing screen output
- How a Makefile builds object files separately, tracks header dependencies and provides the targets `all`, `run`, `test` and `clean`
- Safe input and parsing in C: `fgets` instead of `scanf`, `strtod` with checks, validating text before copying it into a fixed buffer
- Why boundary values matter: limits are inclusive, leap years need a special rule, and the order of the checks in `classifyReading` decides the result
- The Git workflow: commit one piece of work at a time, write meaningful messages, and check what changed before committing or restoring

## Future Improvements

- Save to a temporary file and rename it, so a failed save can never damage existing data
- Use dynamic memory to remove the fixed limits of 50 tags and 1000 readings
- Make the sampling interval configurable and detect duplicate readings
- Add rate-of-change alarms and export trends for plotting
- Add unit tests for the menu logic in `main.c` by separating it further from input and output
