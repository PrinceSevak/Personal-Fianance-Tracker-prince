#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CAPACITY 5
#define DATA_FILE "parking_data.txt"

// Structure for car node
typedef struct Car {
    int slot;
    char regNo[20];
    char owner[30];
    struct Car *next;
} Car;

Car *head = NULL;
int currentCount = 0;

// Function to save data to file
void saveToFile() {
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp) {
        printf("Error opening file!\n");
        return;
    }
    Car *temp = head;
    while (temp != NULL) {
        fprintf(fp, "%d %s %s\n", temp->slot, temp->regNo, temp->owner);
        temp = temp->next;
    }
    fclose(fp);
}

// Function to load data from file
void loadFromFile() {
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp) return;

    while (!feof(fp)) {
        Car *newCar = (Car*)malloc(sizeof(Car));
        if (fscanf(fp, "%d %s %s", &newCar->slot, newCar->regNo, newCar->owner) == 3) {
            newCar->next = head;
            head = newCar;
            currentCount++;
        } else {
            free(newCar);
        }
    }
    fclose(fp);
}

// Function to add a car (Entry)
void parkCar(char regNo[], char owner[]) {
    if (currentCount >= MAX_CAPACITY) {
        printf("🚫 Parking Full! No slots available.\n");
        return;
    }
    Car *newCar = (Car*)malloc(sizeof(Car));
    newCar->slot = currentCount + 1;
    strcpy(newCar->regNo, regNo);
    strcpy(newCar->owner, owner);
    newCar->next = head;
    head = newCar;
    currentCount++;
    printf("✅ Car %s parked at slot %d.\n", regNo, newCar->slot);
    saveToFile();
}

// Function to remove a car (Exit)
void removeCar(char regNo[]) {
    Car *temp = head, *prev = NULL;
    while (temp != NULL && strcmp(temp->regNo, regNo) != 0) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) {
        printf("❌ Car with RegNo %s not found.\n", regNo);
        return;
    }
    if (prev == NULL) head = temp->next;
    else prev->next = temp->next;
    printf("🚗 Car %s exited from slot %d.\n", temp->regNo, temp->slot);
    free(temp);
    currentCount--;
    saveToFile();
}

// Function to display parked cars
void displayCars() {
    if (head == NULL) {
        printf("🅿️ No cars parked.\n");
        return;
    }
    Car *temp = head;
    printf("\n📋 Active Parked Cars:\n");
    while (temp != NULL) {
        printf("Slot %d | RegNo: %s | Owner: %s\n", temp->slot, temp->regNo, temp->owner);
        temp = temp->next;
    }
}

// Menu-driven program
int main() {
    int choice;
    char regNo[20], owner[30];

    loadFromFile();

    do {
        printf("\n===== 🚘 Smart Parking Lot Menu =====\n");
        printf("1. Park Car (Entry)\n");
        printf("2. Remove Car (Exit)\n");
        printf("3. Display Parked Cars\n");
        printf("4. Check Availability\n");
        printf("5. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter Car RegNo: ");
                scanf("%s", regNo);
                printf("Enter Owner Name: ");
                scanf("%s", owner);
                parkCar(regNo, owner);
                break;
            case 2:
                printf("Enter Car RegNo to remove: ");
                scanf("%s", regNo);
                removeCar(regNo);
                break;
            case 3:
                displayCars();
                break;
            case 4:
                printf("Available Slots: %d/%d\n", MAX_CAPACITY - currentCount, MAX_CAPACITY);
                break;
            case 5:
                printf("Exiting... Data saved.\n");
                break;
            default:
                printf("Invalid choice!\n");
        }
    } while (choice != 5);

    return 0;
}
