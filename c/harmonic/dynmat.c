#include <math.h>
#include <stdlib.h>

int get_dynamical_matrix_at_q(double *dynamical_matrix_real,
			      double *dynamical_matrix_imag,
			      const int num_patom, 
			      const int num_satom,
			      const double *fc,
			      const double *q,
			      const double *r,
			      const int *multi,
			      const double *mass,
			      const int *s2p_map, 
			      const int *p2s_map,
			      const double *charge_sum)
{
  int i, j, k, l, m;
  double phase, cos_phase, sin_phase, mass_sqrt, fc_elem;
  double dm_real[3][3], dm_imag[3][3];
  
#pragma omp parallel for private(j, k, l, m, phase, cos_phase, sin_phase, mass_sqrt, dm_real, dm_imag, fc_elem)
  for (i = 0; i < num_patom; i++) {
    for (j = 0; j < num_patom; j++) {
      mass_sqrt = sqrt(mass[i] * mass[j]);

      for (k = 0; k < 3; k++) {
	for (l = 0; l < 3; l++) {
	  dm_real[k][l] = 0;
	  dm_imag[k][l] = 0;
	}
      }

      for (k = 0; k < num_satom; k++) { /* Lattice points of right index of fc */
	if (s2p_map[k] != p2s_map[j]) {
	  continue;
	}

	cos_phase = 0;
	sin_phase = 0;
	for (l = 0; l < multi[k * num_patom + i]; l++) {
	  phase = 0;
	  for (m = 0; m < 3; m++) {
	    phase += q[m] * r[k * num_patom * 81 + i * 81 + l * 3 + m];
	  }
	  cos_phase += cos(phase * 2 * M_PI) / multi[k * num_patom + i];
	  sin_phase += sin(phase * 2 * M_PI) / multi[k * num_patom + i];
	}

	for (l = 0; l < 3; l++) {
	  for (m = 0; m < 3; m++) {
	    if (charge_sum) {
	      fc_elem = (fc[p2s_map[i] * num_satom * 9 + k * 9 + l * 3 + m] +
			 charge_sum[i * num_patom * 9 +
				    j * 9 + l * 3 + m]) / mass_sqrt;
	    } else {
	      fc_elem = fc[p2s_map[i] * num_satom * 9 +
			   k * 9 + l * 3 + m] / mass_sqrt;
	    }
	    dm_real[l][m] += fc_elem * cos_phase;
	    dm_imag[l][m] += fc_elem * sin_phase;
	  }
	}
      }
      
      for (k = 0; k < 3; k++) {
	for (l = 0; l < 3; l++) {
	  dynamical_matrix_real[(i * 3 + k) * num_patom * 3 + j * 3 + l] += dm_real[k][l];
	  dynamical_matrix_imag[(i * 3 + k) * num_patom * 3 + j * 3 + l] += dm_imag[k][l];
	}
      }
    }
  }
  return 0;
}

void get_charge_sum(double *charge_sum,
		    const int num_patom,
		    const double factor,
		    const double q_vector[3],
		    const double *born)
{
  int i, j, k, a, b;
  double (*q_born)[3];
  
  q_born = (double (*)[3]) malloc(sizeof(double[3]) * num_patom);
  for (i = 0; i < num_patom; i++) {
    for (j = 0; j < 3; j++) {
      q_born[i][j] = 0;
    }
  }

  for (i = 0; i < num_patom; i++) {
    for (j = 0; j < 3; j++) {
      for (k = 0; k < 3; k++) {
	q_born[i][j] += q_vector[k] * born[i * 9 + k * 3 + j];
      }
    }
  }

  for (i = 0; i < num_patom; i++) {
    for (j = 0; j < num_patom; j++) {
      for (a = 0; a < 3; a++) {
	for (b = 0; b < 3; b++) {
	  charge_sum[i * 9 * num_patom + j * 9 + a * 3 + b] =
	    q_born[i][a] * q_born[j][b] * factor;
	}
      }
    }
  }

  free(q_born);
}

