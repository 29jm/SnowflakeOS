Contributions are much appreciated!

## Table of contents

+ [Reporting bugs](#reporting-bugs)
+ [Proposing features or improvements](#proposing-features-or-improvements)
+ [Contributing pull requests](#contributing-pull-requests)
+ [Coding guidelines](#coding-guidelines)

## Reporting bugs

+ Open one issue per bug
+ Specify the distro you're using, in particular if it's a build system bug
+ Provide steps to reproduce the bug
+ Provide the expected behavior if it's unclear

## Proposing features or improvements

Please do! Developers tend to develop blind spots after a while, new ideas are welcome. Even if the proposed feature was on the roadmap already, it doesn't hurt to open an issue for it, plus it can be expanded on there.

## Contributing pull requests

+ Make sure there is a corresponding issue in the tracker, create one if needed
+ If you need help, ask
    + Even if you don't, the project owner will pop up after a while with unsolicited advice
+ Read the **coding guidelines** in the [next section](#coding-guidelines) of this page
+ Consult this project's wiki, there are rare cases where it may help
+ Open your PR to get feedback
    + feel free to open it early as a work-in-progress
+ It'll get merged once approved!
    + Intermediate commits may be squashed into one when merging if they're not directly related to the purpose of the PR: you can rebase your branch yourself if you wish to clean its commits yourself

## Coding guidelines

Readability is considered important; try to keep the coding style uniform. Here are the guidelines in use:

+ indent using four spaces
+ use snake_case at all times
+ curly braces on the same line in all cases: `if (foo) {`, etc...
    + put a space before curly braces
+ curly braces for single line `if`s `for`s and `while`s
+ a space between keywords and their parentheses: `if ()`, `for ()`
    + except when they behave like functions: `sizeof()`, `offsetof()`
+ spaces around arithmetic operators: `a + b % c == 0`
+ pointers' `*` are put close to the type: `type_t* ptr = NULL`;
+ a blank line before and after control structures:
  ```c
  int blah = bar();

  if (blah) {
      ...;
  }

  ...;
  ```
    + except when the previous line is also a control structure, e.g. nested loops, an `if` within a loop, etc...
+ no extra blank lines otherwise, or trailing whitespace
+ use sized `ints` within the kernel: `uint32_t`, `uint8_t` etc... or their signed versions if needed
+ prefer lines that fit within 80-90 columns if possible

Example taken from [kernel/src/misc/wm/wm.c](https://github.com/29jm/SnowflakeOS/blob/022fc799b6841aa1365b28ac53832bb2f1cefc2f/kernel/src/misc/wm/wm.c#L406-L419):
```c
/* Return the window iterator corresponding to the given id, NULL if none match.
 */
list_t* wm_get_window(uint32_t id) {
    list_t* iter;
    wm_window_t* win;

    list_for_each(iter, win, &windows) { // counts as a `for` loop
        if (win->id == id) {
            return iter;
        }
    }

    return NULL;
}
```
