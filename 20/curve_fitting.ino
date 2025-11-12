#include <math.h>

// Arduino pin assignment
#define PIN_IR A0

#define DIST_MIN 0
#define DIST_MAX 300
#define DIST_INTERVAL 50

#define DEGREE_MIN 1
#define DEGREE_MAX 5

int pointCount = DIST_MAX / DIST_INTERVAL + 1;

double distances[DIST_MAX / DIST_INTERVAL + 1];
double volts[DIST_MAX / DIST_INTERVAL + 1];

int degree;
double matrix[DEGREE_MAX + 1][DEGREE_MAX + 1];
double vector[DEGREE_MAX + 1];
double coeffs[DEGREE_MAX + 1];

float rSquared = 0; //0 <= r^2 <= 1

void setup()
{
  Serial.begin(1000000);  // initialize serial port

  for(int i = 0; i < pointCount; i++){
    distances[i] = DIST_MIN + DIST_INTERVAL * i;
  }
}

void loop()
{
  unsigned int filtered; // Voltage values from the IR sensor (0 ~ 1023)
  char userInput;

  Serial.print("set the degree[1, 5]: ");
  while(Serial.available() == 0)
    ;
  userInput = Serial.read();
  degree = userInput - '0';
  
  if(degree < DEGREE_MIN || degree > DEGREE_MAX){
    Serial.println("degree out of range");
    return;
  }

  while (Serial.available() == 0)
    ;
  Serial.read();

  for(int i = 0; i < pointCount; i++){
    Serial.print("measured volt at "); Serial.print(distances[i]); Serial.print(": ");
    while(Serial.available() == 0)
      ;
    Serial.read();
    filtered = ir_sensor_filtered(20, 0.5, 0); // Replace n with your desired value
    Serial.println(filtered);
    volts[i] = filtered;
  }

  curve_fitting();
  rSquared = get_R_squared();
  Serial.print("R^2 = "); Serial.println(rSquared);
  
  Serial.print("measuring volt again?(y/n): ");
  do{
    userInput = Serial.read();
  }while(userInput != 'y' && userInput != 'n');
  if(userInput == 'y'){
    return;
  }

  Serial.println("q를 눌러 정확도 검증 종료");
  while(true){
    while(Serial.available() == 0)
      ;
    userInput = Serial.read();
    if(userInput == 'q'){
      break;
    }
    filtered = ir_sensor_filtered(20, 0.5, 0); // Replace n with your desired value
    Serial.print("FLT:"); Serial.print(filtered);
    Serial.print(" ==> Distance:"); Serial.println(volt_to_distance(filtered));
  }
}

double get_polynomial(double x, double* c, int degree){
  double y_predicted = 0.0;
  for (int i = 0; i <= degree; i++) {
    if (i == 0) {
      y_predicted += c[i];
    } else {
      y_predicted += c[i] * pow(x, i); // c[1]*x, c[2]*x^2, ...
    }
  }
  return y_predicted;
}

double get_R_squared(){
  double y_mean = 0.0;
  for(int i = 0; i < pointCount; i++){
    y_mean += distances[i];
  }
  y_mean /= pointCount;

  double ss_tot = 0.0;
  double ss_res = 0.0;

  for(int i = 0; i < pointCount; i++){
    double x_val = volts[i];
    double y_actual = distances[i];

    double y_predicted = get_polynomial(x_val, coeffs, degree);

    ss_tot += pow(y_actual - y_mean, 2);
    ss_res += pow(y_actual - y_predicted, 2);
  }

  double r_squared = 0.0;
  if(ss_tot > 1.0E-12){
    r_squared = 1.0 - (ss_res / ss_tot);
  }

  return r_squared;
}

void curve_fitting(){
  bool isAble = polynomalFit(degree, pointCount, volts, distances, coeffs);

  if(isAble){
    print_equation(degree, coeffs);
  }
  else{
    Serial.println("reset the system and lower your degree");
  }
}

bool polynomalFit(int degree, int count, double *x, double *y, double *resultCoeffs){
  int n = degree + 1;

  int xPowerCount = 2 * degree + 1;
  double xPowers[xPowerCount];
  for(int i = 0; i < xPowerCount; i++){
    xPowers[i] = 0.0;
  }

  for(int i = 0; i < count; i++){
    double val = 1.0;
    for(int j = 0; j < xPowerCount; j++){
      xPowers[j] += val;
      val *= x[i];
    }
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      matrix[i][j] = xPowers[i + j];
    }
  }

  for(int i = 0; i < n; i++){
    vector[i] = 0;
  }

  for(int i = 0; i < count; i++){
    double val = 1.0;
    for(int j = 0; j < n; j++){
      vector[j] += y[i] * val;
      val *= x[i];
    }
  }

  return solve_linear(n, matrix, vector, coeffs);
}

bool solve_linear(int pSize, double A[DEGREE_MAX + 1][DEGREE_MAX + 1], double B[DEGREE_MAX + 1], double results[DEGREE_MAX + 1]){
  for(int i = 0; i < pSize; i++){
    int maxRow = i;
    for(int j = i + 1; j < pSize; j++){
      if(abs(A[j][i]) > abs(A[maxRow][i])){
        maxRow = j;
      }
    }

    for(int j = i; j < pSize; j++){
      double temp = A[i][j];
      A[i][j] = A[maxRow][j];
      A[maxRow][j] = temp;
    }

    double temp = B[i];
    B[i] = B[maxRow];
    B[maxRow] = temp;

    if(abs(A[i][i]) < 1.0E-12){
      return false;
    }

    for(int j = i + 1; j < pSize; j++){
      double factor = A[j][i] / A[i][i];
      for(int k = i; k < pSize; k++){
        A[j][k] -= factor * A[i][k];
      }
      B[j] -= factor * B[i];
    }
  }

  for(int i = pSize - 1; i >= 0; i--){
    results[i] = B[i];
    for(int j = i + 1; j < pSize; j++){
      results[i] -= A[i][j] * results[j];
    }
    results[i] /= A[i][i];
  }

  return true;
}

void print_equation(int degree, double* coeffs){
  Serial.print("equation = ");

  for(int i = 0; i <= degree; i++){
    if(abs(coeffs[i]) < 1.0E-12){
      continue;
    }
    if(i == 0){
      if(coeffs[i] < 0){
        Serial.print("-");
      }
    }
    else{
      Serial.print(coeffs[i] > 0? " + " : " - ");
    }

    Serial.print(abs(coeffs[i]), 8);

    if(i >= 1){
      Serial.print(" * x");
      if(i > 1){
        Serial.print(" ^ ");
        Serial.print(i);
      }
    }
  }
  Serial.println();
}

float volt_to_distance(unsigned int a_value) 
{
  // Replace below line with the equation obtained from nonlinear regression analysis
  //return (6762.0 / (a_value - 9) - 4.0) * 10.0;
  //return coodet_const + (coodet_first * a_value) + (coodet_quad * a_value * a_value);
  float sum = 0;
  for(int i = 0; i <= degree; i++){
    float temp = coeffs[i];
    for(int j = 0; j < i; j++){
      temp *= a_value;
    }
    sum += temp;
  }
  return sum;
}

int compare(const void *a, const void *b) {
  return (*(unsigned int *)a - *(unsigned int *)b);
}

unsigned int ir_sensor_filtered(unsigned int n, float position, int verbose)
{
  // Eliminate spiky noise of an IR distance sensor by repeating measurement and taking a middle value
  // n: number of measurement repetition
  // position: the percentile of the sample to be taken (0.0 <= position <= 1.0)
  // verbose: 0 - normal operation, 1 - observing the internal procedures, and 2 - calculating elapsed time.
  // Example 1: ir_sensor_filtered(n, 0.5, 0) => return the median value among the n measured samples.
  // Example 2: ir_sensor_filtered(n, 0.0, 0) => return the smallest value among the n measured samples.
  // Example 3: ir_sensor_filtered(n, 1.0, 0) => return the largest value among the n measured samples.

  // The output of Sharp infrared sensor includes lots of spiky noise.
  // To eliminate such a spike, ir_sensor_filtered() performs the following two steps:
  // Step 1. Repeat measurement n times and collect n * position smallest samples, where 0 <= postion <= 1.
  // Step 2. Return the position'th sample after sorting the collected samples.

  // returns 0, if any error occurs

  unsigned int *ir_val, ret_val;
  unsigned int start_time;
 
  if (verbose >= 2)
    start_time = millis(); 

  if ((n == 0) || (n > 100) || (position < 0.0) || (position > 1))
    return 0;
    
  if (position == 1.0)
    position = 0.999;

  if (verbose == 1) {
    Serial.print("n: "); Serial.print(n);
    Serial.print(", position: "); Serial.print(position); 
    Serial.print(", ret_idx: ");  Serial.println((unsigned int)(n * position)); 
  }

  ir_val = (unsigned int *)malloc(sizeof(unsigned int) * n);
  if (ir_val == NULL)
    return 0;

  if (verbose == 1)
    Serial.print("IR:");
  
  for (int i = 0; i < n; i++) {
    ir_val[i] = analogRead(PIN_IR);
    if (verbose == 1) {
        Serial.print(" ");
        Serial.print(ir_val[i]);
    }
  }

  if (verbose == 1)
    Serial.print  ("  => ");

  qsort(ir_val, n, sizeof(unsigned int), compare);
  ret_val = ir_val[(unsigned int)(n * position)];

  if (verbose == 1) {
    for (int i = 0; i < n; i++) {
        Serial.print(" ");
        Serial.print(ir_val[i]);
    }
    Serial.print(" :: ");
    Serial.println(ret_val);
  }
  free(ir_val);

  if (verbose >= 2) {
    Serial.print("Elapsed time: "); Serial.print(millis() - start_time); Serial.println("ms");
  }
  
  return ret_val;
}
