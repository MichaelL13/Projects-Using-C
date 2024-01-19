
#ifndef HEADER_H
#define HEADER_H

typedef struct Order {
    unsigned id;
    unsigned arrivalTime;
    unsigned completionTime;
    unsigned deliverTime;
    int how_many_pizzas;
} Order;

enum {
    NS_PER_SECOND = 1000000000
};
  
#define N_COOK 2
#define N_OVEN 5
#define N_DELIVERERS 10
#define T_ORDER_LOW 1
#define T_ORDER_HIGH 5
#define N_ORDER_LOW 1
#define N_ORDER_HIGH 5
#define T_PREP 1
#define T_BAKE 10
#define T_LOW 5
#define T_HIGH 15

#define T0 0


 

#endif /* HEADER_H */

