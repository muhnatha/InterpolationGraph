#include <stdio.h>   
#include <stdlib.h>  

// points data type in 2D 
typedef struct {
    float x;
    float y;
}point;

int main(){
    // number of points
    int n;

    // ask for number of points
    printf("How many points : ");
    scanf("%d", &n);
    point points[n];

    // input points
    printf("input %d points (x, y)\n", n);
    for(int i=0; i<n; i++){
        scanf("%f %f", &points[i].x, &points[i].y);
    }

    // chech points
    //for(int i=0;  i<n; i++){
    //    printf("\n%.2f %.2f",points[i].x, points[i].y);
    //}

}



