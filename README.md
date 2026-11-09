# Software Dependency and License Compliance Auditor

A self-contained C++17 command-line program. The sample dependencies and
license policy are built into `main.cpp`, so no input files or build system
are required. Unknown or blank licenses are marked `REVIEW`.

## Build and run

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o dependency_auditor.exe
.\dependency_auditor.exe
```

On Windows, you can also compile with MSVC from a Developer Command Prompt:

```powershell
cl /std:c++17 /EHsc /W4 main.cpp
.\main.exe
```

To use your own CSV inputs and optionally save a report:

```powershell
.\dependency_auditor.exe dependencies.csv license_policy.csv audit_report.csv
```

The program returns exit code `0` when every dependency is allowed, `2` when
any dependency needs review or is prohibited, and `1` for invalid input or
file errors.

## Optional CSV input formats

`dependencies.csv` must contain `name`, `version`, and `license` columns.
`license_policy.csv` must contain `license` and `status` columns. Column names
are case-insensitive. CSV fields can be quoted; double quotes inside a quoted
field are escaped by doubling them.

Policy statuses are `allowed`, `review`, or `prohibited`. License matching is
case-insensitive and exact after trimming whitespace. Licenses missing from
the policy default to `REVIEW`; this tool reports policy matches and does not
provide legal advice.

## C++ concepts demonstrated

- Classes/structs and constructors (`Dependency`, `AuditResult`)
- STL containers (`vector`, `map`) and algorithms (`sort`, `count_if`)
- String processing and sorting
- CSV file input/output and report generation
- Exceptions and input validation
