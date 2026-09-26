# Assignment 1 — Student Record Management System (Linked List)

**Name:** Uma Bharathi D  
**SR Number:** 25964  
**Course:** E3-257 Embedded System Design

## Description
A menu-driven C program that uses a singly linked list to store student records. Each record
contains Roll Number, Name, Gender and CGPA. Records are loaded from a CSV file at start-up.

## Features
- Read student records from a CSV file
- Add a new student (duplicate roll numbers are rejected)
- Remove a student by Roll Number
- Modify student details
- Search by Roll Number / Name / Gender
- Display students within a CGPA range
- Sort students by CGPA or by Roll Number
- Display all student records

## Files
| File | Purpose |
|---|---|
| `demo.c` | C source code |
| `students.csv` | Input data (roll, name, gender, CGPA) |

## Build & Run
```bash
gcc demo.c -o demo
./demo          # on Windows: demo.exe
```
`students.csv` must be in the same directory the program is run from.
