This directory contains implementations for west commands which are
tightly coupled to the wellsl4 tree. Currently, those are the build,
flash, and debug commands.

Before adding more here, consider whether you might want to put new
extensions in upstream west. For example, any commands which operate
on the multi-repo need to be in upstream west, not here. Try to limit
what goes in here to just those files that change along with WellL4
itself.

When extending this code, please keep the unit tests (in tests/) up to
date. You can run the tests with this command from this directory:

$ PYTHONPATH=$PWD py.test

Windows users will need to find the path to .west/west/src in their
WellL4 installation, then run something like this:

> cmd /C "set PYTHONPATH=path\to\wellsl4\scripts\west_commands && py.test"

Note that these tests are run as part of WellL4's CI when submitting
an upstream pull request, and pull requests which break the tests
cannot be merged.

Thanks!
