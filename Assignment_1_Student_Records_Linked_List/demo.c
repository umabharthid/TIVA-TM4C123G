#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Student {
    int roll;
    char name[50];
    char gender;
    float cgpa;
    struct Student *next;
};

struct Student *head = NULL;

/* Function declarations */
void loadFromCSV();
void saveToCSV();
void clearInputBuffer();

int rollExists(int roll);

void insertStudent();
void deleteStudent();
void modifyStudent();
void searchStudent();
void displayByCGPA();
void sortByCGPA();
void sortByRoll();
void displayAll();

/* ===================== MAIN ===================== */
int main() {
    int choice;

    loadFromCSV();

    while (1) {
        printf("\n===== MENU =====\n");
        printf("1. Add Student\n");
        printf("2. Remove Student (Roll No)\n");
        printf("3. Modify Student\n");
        printf("4. Search Student\n");
        printf("5. Display by CGPA Range\n");
        printf("6. Sort by CGPA\n");
        printf("7. Sort by Roll Number\n");
        printf("8. Display All Students\n");
        printf("9. Exit\n");
        printf("Enter choice number: ");
        scanf("%d", &choice);
        clearInputBuffer();

        switch (choice) {
            case 1: insertStudent(); break;
            case 2: deleteStudent(); break;
            case 3: modifyStudent(); break;
            case 4: searchStudent(); break;
            case 5: displayByCGPA(); break;
            case 6: sortByCGPA(); break;
            case 7: sortByRoll(); break;
            case 8: displayAll(); break;
            case 9:
                saveToCSV();
                exit(0);
            default:
                printf("Invalid choice!\n");
        }
    }
}

/* ================= INPUT BUFFER ================= */
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ================= CSV LOAD ===================== */
void loadFromCSV() {
    FILE *fp = fopen("students.csv", "r");
    if (!fp) {
        printf("CSV file not found. Starting fresh.\n");
        return;
    }

    while (1) {
        struct Student *temp = malloc(sizeof(struct Student));
        if (fscanf(fp, "%d,%49[^,],%c,%f\n",
                   &temp->roll,
                   temp->name,
                   &temp->gender,
                   &temp->cgpa) != 4) {
            free(temp);
            break;
        }

        temp->next = head;
        head = temp;
    }
    fclose(fp);
}

/* ================= CSV SAVE ===================== */
void saveToCSV() {
    FILE *fp = fopen("students.csv", "w");
    if (!fp) {
        printf("Error saving CSV file.\n");
        return;
    }

    struct Student *temp = head;
    while (temp) {
        fprintf(fp, "%d,%s,%c,%.2f\n",
                temp->roll,
                temp->name,
                temp->gender,
                temp->cgpa);
        temp = temp->next;
    }
    fclose(fp);
}

/* ================= ROLL CHECK =================== */
int rollExists(int roll) {
    struct Student *temp = head;
    while (temp) {
        if (temp->roll == roll)
            return 1;
        temp = temp->next;
    }
    return 0;
}

/* ================= INSERT ======================= */
void insertStudent() {
    struct Student *newNode = malloc(sizeof(struct Student));

    printf("Roll No: ");
    scanf("%d", &newNode->roll);
    clearInputBuffer();

    if (rollExists(newNode->roll)) {
        printf("Roll number already exists.\n");
        free(newNode);
        return;
    }

    printf("Name: ");
    fgets(newNode->name, 50, stdin);
    newNode->name[strcspn(newNode->name, "\n")] = 0;

    printf("Gender (M/F): ");
    scanf(" %c", &newNode->gender);
    clearInputBuffer();

    printf("CGPA: ");
    scanf("%f", &newNode->cgpa);
    clearInputBuffer();

    newNode->next = head;
    head = newNode;

    saveToCSV();
    printf("Student added successfully.\n");
}

/* ================= DELETE ======================= */
void deleteStudent() {
    int roll;
    printf("Enter Roll No to delete: ");
    scanf("%d", &roll);
    clearInputBuffer();

    struct Student *temp = head, *prev = NULL;

    while (temp && temp->roll != roll) {
        prev = temp;
        temp = temp->next;
    }

    if (!temp) {
        printf("Student not found.\n");
        return;
    }

    if (!prev)
        head = temp->next;
    else
        prev->next = temp->next;

    free(temp);
    saveToCSV();
    printf("Student removed.\n");
}

/* ================= MODIFY ======================= */
void modifyStudent() {
    int roll;
    printf("Enter Roll No to modify: ");
    scanf("%d", &roll);
    clearInputBuffer();

    struct Student *temp = head;
    while (temp && temp->roll != roll)
        temp = temp->next;

    if (!temp) {
        printf("Student not found.\n");
        return;
    }

    printf("New Name: ");
    fgets(temp->name, 50, stdin);
    temp->name[strcspn(temp->name, "\n")] = 0;

    printf("New Gender (M/F): ");
    scanf(" %c", &temp->gender);
    clearInputBuffer();

    printf("New CGPA: ");
    scanf("%f", &temp->cgpa);
    clearInputBuffer();

    saveToCSV();
    printf("Student updated.\n");
}

/* ================= SEARCH ======================= */
void searchStudent() {
    int ch, roll;
    char key[50], gender;

    printf("Search by: 1.Roll  2.Name  3.Gender : ");
    scanf("%d", &ch);
    clearInputBuffer();

    struct Student *temp = head;

    if (ch == 1) {
        printf("Enter Roll No: ");
        scanf("%d", &roll);
        clearInputBuffer();

        while (temp) {
            if (temp->roll == roll) {
                printf("%d %s %c %.2f\n",
                       temp->roll, temp->name,
                       temp->gender, temp->cgpa);
                return;
            }
            temp = temp->next;
        }
        printf("Student not found.\n");
    }

    else if (ch == 2) {
        printf("Enter Name: ");
        fgets(key, 50, stdin);
        key[strcspn(key, "\n")] = 0;

        while (temp) {
            if (strcmp(temp->name, key) == 0)
                printf("%d %s %c %.2f\n",
                       temp->roll, temp->name,
                       temp->gender, temp->cgpa);
            temp = temp->next;
        }
    }

    else if (ch == 3) {
        printf("Enter Gender (M/F): ");
        scanf(" %c", &gender);
        clearInputBuffer();

        while (temp) {
            if (temp->gender == gender)
                printf("%d %s %c %.2f\n",
                       temp->roll, temp->name,
                       temp->gender, temp->cgpa);
            temp = temp->next;
        }
    }
}

/* ================= DISPLAY CGPA ================= */
void displayByCGPA() {
    float low, high;
    printf("Enter CGPA range: ");
    scanf("%f %f", &low, &high);
    clearInputBuffer();

    struct Student *temp = head;
    while (temp) {
        if (temp->cgpa >= low && temp->cgpa <= high)
            printf("%d %s %c %.2f\n",
                   temp->roll, temp->name,
                   temp->gender, temp->cgpa);
        temp = temp->next;
    }
}

/* ================= SORT CGPA ==================== */
void sortByCGPA() {
    int order;
    printf("1. Ascending  2. Descending: ");
    scanf("%d", &order);
    clearInputBuffer();

    struct Student *i, *j;
    for (i = head; i && i->next; i = i->next) {
        for (j = i->next; j; j = j->next) {
            if ((order == 1 && i->cgpa > j->cgpa) ||
                (order == 2 && i->cgpa < j->cgpa)) {

                int r = i->roll; float c = i->cgpa; char g = i->gender;
                char n[50]; strcpy(n, i->name);

                i->roll = j->roll; i->cgpa = j->cgpa;
                i->gender = j->gender; strcpy(i->name, j->name);

                j->roll = r; j->cgpa = c;
                j->gender = g; strcpy(j->name, n);
            }
        }
    }
    saveToCSV();
    printf("Sorted by CGPA.\n");
}

/* ================= SORT ROLL ==================== */
void sortByRoll() {
    int order;
    printf("1. Ascending  2. Descending: ");
    scanf("%d", &order);
    clearInputBuffer();

    struct Student *i, *j;
    for (i = head; i && i->next; i = i->next) {
        for (j = i->next; j; j = j->next) {
            if ((order == 1 && i->roll > j->roll) ||
                (order == 2 && i->roll < j->roll)) {

                int r = i->roll; float c = i->cgpa; char g = i->gender;
                char n[50]; strcpy(n, i->name);

                i->roll = j->roll; i->cgpa = j->cgpa;
                i->gender = j->gender; strcpy(i->name, j->name);

                j->roll = r; j->cgpa = c;
                j->gender = g; strcpy(j->name, n);
            }
        }
    }
    saveToCSV();
    printf("Sorted by Roll Number.\n");
}

/* ================= DISPLAY ALL ================== */
void displayAll() {
    struct Student *temp = head;
    printf("\nRoll  Name                 Gender CGPA\n");
    printf("-------------------------------------------\n");
    while (temp) {
        printf("%-5d %-20s %-6c %.2f\n",
               temp->roll,
               temp->name,
               temp->gender,
               temp->cgpa);
        temp = temp->next;
    }
}
