// #include "scalers.h"


// Picture* byXtoY(Picture* by, int scale_factor, int new_rows, int new_cols) {

//     int start_x = 0;//scale_factor / 2;
//     int start_y = 0;//scale_factor / 2;
//     uint8_t* res = (uint8_t*)malloc(new_rows * new_cols * by->bytes_per_pixel() * sizeof(unint8_t));
//     int i = 0;
//     int j = 0;
//     int a = 0;
//     int b = 0;

//     ulong buffer = 0;
//     for (i = 0; i < new_rows; i += scale_factor){
//         for (j = 0; j < new_cols; j += scale_factor){

//             for (a = 0; a < scale_factor; a++){
//                 for (b = 0; b < scale_factor; b++){

//                 }
//             }
//         }
//     }

// }

// Picture* scale_by240to48(Picture* by) {
//     return byXtoY(by, 5, 48, 48);
// }