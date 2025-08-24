#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX 100

struct Transaction {
    int id;
    char type[10];      // "Income" or "Expense"
    char category[20];
    float amount;
    char date[15];      // format: YYYY-MM-DD
};

struct Transaction t[MAX];
int count = 0;
float savingsGoal = 0.0;

// Function Prototypes
void addTransaction();
void displayTransactions();
void filterExpenses();
void sortByAmount();
void searchByCategory();
void saveToFile();
void loadFromFile();
void barChart();
void setSavingsGoal();
void showSavingsProgress();

int main() {
    int choice;
    loadFromFile(); // Load existing data at start

    do {
        printf("\n=== Personal Finance Tracker ===\n");
        printf("1. Add Transaction\n");
        printf("2. Display All Transactions\n");
        printf("3. Filter Expenses > $100\n");
        printf("4. Sort by Amount\n");
        printf("5. Search by Category\n");
        printf("6. Save to File\n");
        printf("7. Show Monthly Spending Bar Chart\n");
        printf("8. Set Savings Goal\n");
        printf("9. Show Savings Progress\n");
        printf("0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch(choice) {
            case 1: addTransaction(); break;
            case 2: displayTransactions(); break;
            case 3: filterExpenses(); break;
            case 4: sortByAmount(); break;
            case 5: searchByCategory(); break;
            case 6: saveToFile(); break;
            case 7: barChart(); break;
            case 8: setSavingsGoal(); break;
            case 9: showSavingsProgress(); break;
            case 0: saveToFile(); printf("Exiting...\n"); break;
            default: printf("Invalid choice!\n");
        }
    } while(choice != 0);

    return 0;
}

void addTransaction() {
    if(count >= MAX) {
        printf("Transaction limit reached!\n");
        return;
    }
    struct Transaction temp;
    temp.id = count + 1;

    printf("Enter type (Income/Expense): ");
    scanf("%s", temp.type);
    printf("Enter category: ");
    scanf("%s", temp.category);
    printf("Enter amount: ");
    scanf("%f", &temp.amount);
    printf("Enter date (YYYY-MM-DD): ");
    scanf("%s", temp.date);

    t[count++] = temp;
    printf("Transaction added!\n");
}

void displayTransactions() {
    if(count == 0) {
        printf("No transactions yet.\n");
        return;
    }
    printf("\nID  Type     Category     Amount     Date\n");
    printf("---------------------------------------------\n");
    for(int i=0; i<count; i++) {
        printf("%-3d %-8s %-12s $%-8.2f %s\n", t[i].id, t[i].type, t[i].category, t[i].amount, t[i].date);
    }
}

void filterExpenses() {
    printf("\nExpenses greater than $100:\n");
    for(int i=0; i<count; i++) {
        if(strcmp(t[i].type, "Expense") == 0 && t[i].amount > 100) {
            printf("%d %s %s $%.2f %s\n", t[i].id, t[i].type, t[i].category, t[i].amount, t[i].date);
        }
    }
}

void sortByAmount() {
    for(int i=0; i<count-1; i++) {
        for(int j=i+1; j<count; j++) {
            if(t[i].amount > t[j].amount) {
                struct Transaction temp = t[i];
                t[i] = t[j];
                t[j] = temp;
            }
        }
    }
    printf("Transactions sorted by amount!\n");
    displayTransactions();
}

void searchByCategory() {
    char cat[20];
    printf("Enter category to search: ");
    scanf("%s", cat);
    int found = 0;
    for(int i=0; i<count; i++) {
        if(strcmp(t[i].category, cat) == 0) {
            printf("%d %s %s $%.2f %s\n", t[i].id, t[i].type, t[i].category, t[i].amount, t[i].date);
            found = 1;
        }
    }
    if(!found) printf("No transactions found in this category.\n");
}

void saveToFile() {
    FILE *fp = fopen("transactions.txt", "w");
    if(!fp) {
        printf("Error saving file!\n");
        return;
    }
    fprintf(fp, "SAVINGS_GOAL %.2f\n", savingsGoal);
    for(int i=0; i<count; i++) {
        fprintf(fp, "%d %s %s %.2f %s\n", t[i].id, t[i].type, t[i].category, t[i].amount, t[i].date);
    }
    fclose(fp);
    printf("Data saved to file.\n");
}

void loadFromFile() {
    FILE *fp = fopen("transactions.txt", "r");
    if(!fp) return;

    char firstWord[20];
    if(fscanf(fp, "%s", firstWord) == 1 && strcmp(firstWord, "SAVINGS_GOAL") == 0) {
        fscanf(fp, "%f", &savingsGoal);
    } else {
        rewind(fp);
    }

    while(fscanf(fp, "%d %s %s %f %s", &t[count].id, t[count].type, t[count].category, &t[count].amount, t[count].date) == 5) {
        count++;
    }
    fclose(fp);
}

void barChart() {
    printf("\nMonthly Spending (ASCII Bar Chart)\n");
    printf("-----------------------------------\n");

    float monthly[13] = {0};
    for(int i=0; i<count; i++) {
        if(strcmp(t[i].type, "Expense") == 0) {
            int month;
            sscanf(t[i].date, "%*d-%d-%*d", &month); // extract month
            monthly[month] += t[i].amount;
        }
    }

    for(int m=1; m<=12; m++) {
        if(monthly[m] > 0) {
            printf("Month %2d | ", m);
            int bars = (int)(monthly[m] / 50); // scale factor
            for(int j=0; j<bars; j++) printf("#");
            printf(" (%.2f)\n", monthly[m]);
        }
    }
}

void setSavingsGoal() {
    printf("Enter your savings goal: ");
    scanf("%f", &savingsGoal);
    printf("Savings goal set to $%.2f\n", savingsGoal);
}

void showSavingsProgress() {
    float income = 0, expense = 0;
    for(int i=0; i<count; i++) {
        if(strcmp(t[i].type, "Income") == 0) income += t[i].amount;
        else if(strcmp(t[i].type, "Expense") == 0) expense += t[i].amount;
    }
    float savings = income - expense;

    printf("\nSavings Progress:\n");
    printf("Total Income: $%.2f\n", income);
    printf("Total Expense: $%.2f\n", expense);
    printf("Current Savings: $%.2f\n", savings);
    if(savingsGoal > 0) {
        printf("Savings Goal: $%.2f\n", savingsGoal);
        float percent = (savings / savingsGoal) * 100;
        if(percent > 100) percent = 100;
        printf("Progress: %.2f%%\n", percent);
    } else {
        printf("No savings goal set.\n");
    }
}
