#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DRIVERS 200
#define MAX_RIDERS  200
#define MAX_RIDES   500

/* -----------------------------
   Models
--------------------------------*/
typedef struct {
    int id;
    char name[32];
    float x, y;       // location
    float rating;     // 0.0 - 5.0
    int available;    // 1 = available, 0 = busy
} Driver;

typedef struct {
    int id;
    char name[32];
    float x, y;       // pickup location
} Rider;

typedef struct {
    int rideId;
    int riderId;
    int driverId;
    float distance;
    float fare;
} Ride;

/* -----------------------------
   Global storage
--------------------------------*/
Driver drivers[MAX_DRIVERS];
int driverCount = 0;

Ride rides[MAX_RIDES];
int rideCount = 0;

int nextRiderId = 1;
int nextDriverId = 1;
int nextRideId = 1;

/* -----------------------------
   Rider Queue (circular)
--------------------------------*/
typedef struct {
    Rider buf[MAX_RIDERS];
    int head; // index of front
    int tail; // next insert pos
    int size;
} RiderQueue;

void rq_init(RiderQueue *q) { q->head = q->tail = q->size = 0; }
int  rq_empty(RiderQueue *q) { return q->size == 0; }
int  rq_full (RiderQueue *q) { return q->size == MAX_RIDERS; }

int rq_enqueue(RiderQueue *q, Rider r) {
    if (rq_full(q)) return 0;
    q->buf[q->tail] = r;
    q->tail = (q->tail + 1) % MAX_RIDERS;
    q->size++;
    return 1;
}

int rq_front(RiderQueue *q, Rider *out) {
    if (rq_empty(q)) return 0;
    *out = q->buf[q->head];
    return 1;
}

int rq_dequeue(RiderQueue *q, Rider *out) {
    if (rq_empty(q)) return 0;
    *out = q->buf[q->head];
    q->head = (q->head + 1) % MAX_RIDERS;
    q->size--;
    return 1;
}

/* -----------------------------
   Driver Priority Queue (min-heap)
   Key: (distance asc, rating desc)
   We precompute distance for the current rider
--------------------------------*/
typedef struct {
    int driverIndex;   // index in drivers[]
    float distance;    // distance to current rider
    float rating;      // driver rating (for tie-break)
} DriverPQItem;

typedef struct {
    DriverPQItem heap[MAX_DRIVERS];
    int size;
} DriverPQ;

int pq_better(DriverPQItem a, DriverPQItem b) {
    if (a.distance < b.distance) return 1;
    if (a.distance > b.distance) return 0;
    // tie on distance -> higher rating first
    return a.rating > b.rating;
}

void pq_init(DriverPQ *pq) { pq->size = 0; }

void pq_swap(DriverPQItem *a, DriverPQItem *b) {
    DriverPQItem tmp = *a; *a = *b; *b = tmp;
}

void pq_push(DriverPQ *pq, DriverPQItem item) {
    int i = ++pq->size;
    pq->heap[i] = item;
    // up-heap
    while (i > 1) {
        int p = i / 2;
        if (pq_better(pq->heap[i], pq->heap[p])) {
            pq_swap(&pq->heap[i], &pq->heap[p]);
            i = p;
        } else break;
    }
}

int pq_empty(DriverPQ *pq) { return pq->size == 0; }

DriverPQItem pq_top(DriverPQ *pq) { return pq->heap[1]; }

DriverPQItem pq_pop(DriverPQ *pq) {
    DriverPQItem ret = pq->heap[1];
    pq->heap[1] = pq->heap[pq->size--];
    // down-heap
    int i = 1;
    while (1) {
        int l = 2*i, r = 2*i + 1, best = i;
        if (l <= pq->size && pq_better(pq->heap[l], pq->heap[best])) best = l;
        if (r <= pq->size && pq_better(pq->heap[r], pq->heap[best])) best = r;
        if (best != i) {
            pq_swap(&pq->heap[i], &pq->heap[best]);
            i = best;
        } else break;
    }
    return ret;
}

/* -----------------------------
   Utilities
--------------------------------*/
float dist(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2, dy = y1 - y2;
    return sqrtf(dx*dx + dy*dy);
}

void pause_enter() {
    printf("\nPress ENTER to continue...");
    int c; while ((c = getchar()) != '\n' && c != EOF) {}
    getchar();
}

/* -----------------------------
   Core: Build driver PQ for a rider
--------------------------------*/
void build_driver_pq_for_rider(const Rider *r, DriverPQ *pq) {
    pq_init(pq);
    for (int i = 0; i < driverCount; i++) {
        if (drivers[i].available) {
            DriverPQItem it;
            it.driverIndex = i;
            it.distance = dist(r->x, r->y, drivers[i].x, drivers[i].y);
            it.rating = drivers[i].rating;
            pq_push(pq, it);
        }
    }
}

/* -----------------------------
   Ride history helpers
--------------------------------*/
void record_ride(int riderId, int driverId, float distance, float fare) {
    if (rideCount >= MAX_RIDES) return;
    rides[rideCount++] = (Ride){ .rideId = nextRideId++, .riderId = riderId,
                                 .driverId = driverId, .distance = distance, .fare = fare };
}

/* -----------------------------
   App state
--------------------------------*/
RiderQueue riderQueue;

/* -----------------------------
   Menu actions
--------------------------------*/
void action_add_driver() {
    if (driverCount >= MAX_DRIVERS) {
        printf("Driver capacity reached.\n");
        return;
    }
    Driver d;
    d.id = nextDriverId++;
    printf("Driver name: "); scanf("%31s", d.name);
    printf("Driver location x y (e.g., 3.5 7.2): "); scanf("%f %f", &d.x, &d.y);
    printf("Driver rating (0.0 - 5.0): "); scanf("%f", &d.rating);
    d.available = 1;
    drivers[driverCount++] = d;
    printf("Added Driver #%d (%s) at (%.2f, %.2f), rating %.1f, available\n",
           d.id, d.name, d.x, d.y, d.rating);
}

void action_list_drivers() {
    if (driverCount == 0) { printf("No drivers.\n"); return; }
    printf("\n-- Drivers --\n");
    printf("ID   Name            Loc(x,y)     Rating  Status\n");
    for (int i = 0; i < driverCount; i++) {
        printf("%-4d %-15s (%6.2f,%6.2f)  %5.1f   %s\n",
               drivers[i].id, drivers[i].name, drivers[i].x, drivers[i].y,
               drivers[i].rating, drivers[i].available ? "Available" : "Busy");
    }
}

void action_add_rider() {
    if (rq_full(&riderQueue)) { printf("Rider queue full.\n"); return; }
    Rider r;
    r.id = nextRiderId++;
    printf("Rider name: "); scanf("%31s", r.name);
    printf("Pickup location x y: "); scanf("%f %f", &r.x, &r.y);
    if (!rq_enqueue(&riderQueue, r)) {
        printf("Failed to enqueue rider.\n");
        return;
    }
    printf("Added Rider #%d (%s) pickup at (%.2f, %.2f)\n", r.id, r.name, r.x, r.y);
}

void action_show_rider_queue() {
    if (rq_empty(&riderQueue)) { printf("Rider queue is empty.\n"); return; }
    printf("\n-- Rider Queue (front -> back) --\n");
    int idx = riderQueue.head;
    for (int k = 0; k < riderQueue.size; k++) {
        Rider *r = &riderQueue.buf[idx];
        printf("Rider %-3d %-15s (%6.2f,%6.2f)\n", r->id, r->name, r->x, r->y);
        idx = (idx + 1) % MAX_RIDERS;
    }
}

float estimate_fare(float distance) {
    const float base = 30.0f;      // base fare
    const float per_km = 12.0f;    // per km
    return base + per_km * distance;
}

int dispatch_one() {
    if (rq_empty(&riderQueue)) {
        printf("No riders waiting.\n");
        return 0;
    }

    // Peek the front rider (don't remove yet)
    Rider rfront;
    rq_front(&riderQueue, &rfront);

    // Build PQ of available drivers for this rider
    DriverPQ pq;
    build_driver_pq_for_rider(&rfront, &pq);

    if (pq_empty(&pq)) {
        printf("No available drivers for Rider #%d (%s) right now. Try later.\n",
               rfront.id, rfront.name);
        return 0; // rider stays in queue
    }

    // Choose best driver
    DriverPQItem best = pq_pop(&pq);
    int di = best.driverIndex;
    Driver *d = &drivers[di];

    // Now dequeue rider (committing the assignment)
    Rider r;
    rq_dequeue(&riderQueue, &r);

    // Mark driver busy and move driver to rider location (picked up)
    d->available = 0;
    d->x = r.x; d->y = r.y;

    float km = best.distance; // treat units as km for demo
    float fare = estimate_fare(km);

    record_ride(r.id, d->id, km, fare);

    printf("Assigned Driver #%d (%s, %.1f★) to Rider #%d (%s). "
           "Distance: %.2f km, Fare: ₹%.2f\n",
           d->id, d->name, d->rating, r.id, r.name, km, fare);

    return 1;
}

void action_dispatch_one() { (void)dispatch_one(); }

void action_dispatch_all() {
    int count = 0;
    while (dispatch_one()) count++;
    if (count == 0) printf("No rides dispatched.\n");
    else printf("Dispatched %d ride(s).\n", count);
}

void action_show_ride_history() {
    if (rideCount == 0) { printf("No rides yet.\n"); return; }
    printf("\n-- Ride History --\n");
    printf("RideID  RiderID  DriverID  Distance(km)  Fare\n");
    for (int i = 0; i < rideCount; i++) {
        printf("%-7d %-8d %-9d %12.2f   ₹%.2f\n",
               rides[i].rideId, rides[i].riderId, rides[i].driverId,
               rides[i].distance, rides[i].fare);
    }
}

void action_toggle_driver_status() {
    int id; printf("Enter Driver ID to toggle availability: ");
    if (scanf("%d", &id) != 1) return;
    for (int i = 0; i < driverCount; i++) {
        if (drivers[i].id == id) {
            drivers[i].available = !drivers[i].available;
            printf("Driver #%d is now %s.\n", id, drivers[i].available ? "Available" : "Busy");
            return;
        }
    }
    printf("Driver not found.\n");
}

void action_save_history_csv() {
    FILE *fp = fopen("rides.csv", "w");
    if (!fp) { printf("Failed to open rides.csv for writing.\n"); return; }
    fprintf(fp, "ride_id,rider_id,driver_id,distance_km,fare\n");
    for (int i = 0; i < rideCount; i++) {
        fprintf(fp, "%d,%d,%d,%.2f,%.2f\n",
                rides[i].rideId, rides[i].riderId, rides[i].driverId,
                rides[i].distance, rides[i].fare);
    }
    fclose(fp);
    printf("Saved %d rides to rides.csv\n", rideCount);
}

/* -----------------------------
   Menu loop
--------------------------------*/
void print_menu() {
    printf("\n=== Ride-Sharing Dispatch Simulator ===\n");
    printf("1. Add Driver\n");
    printf("2. List Drivers\n");
    printf("3. Add Rider Request (enqueue)\n");
    printf("4. Show Rider Queue\n");
    printf("5. Dispatch ONE\n");
    printf("6. Dispatch ALL\n");
    printf("7. Show Ride History\n");
    printf("8. Toggle Driver Availability\n");
    printf("9. Save Ride History to CSV\n");
    printf("0. Exit\n");
    printf("Select: ");
}

int main() {
    rq_init(&riderQueue);
    int choice;
    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: action_add_driver(); break;
            case 2: action_list_drivers(); break;
            case 3: action_add_rider(); break;
            case 4: action_show_rider_queue(); break;
            case 5: action_dispatch_one(); break;
            case 6: action_dispatch_all(); break;
            case 7: action_show_ride_history(); break;
            case 8: action_toggle_driver_status(); break;
            case 9: action_save_history_csv(); break;
            case 0: printf("Bye!\n"); return 0;
            default: printf("Invalid option.\n"); break;
        }
    }
    return 0;
}
