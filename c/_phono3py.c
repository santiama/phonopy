#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <numpy/arrayobject.h>
#include <lapacke.h>
#include "lapack_wrapper.h"
#include "interaction.h"
#include "phonoc_array.h"
#include "imag_self_energy.h"

static PyObject * py_get_jointDOS(PyObject *self, PyObject *args);

static PyObject * py_get_interaction(PyObject *self, PyObject *args);
static PyObject * py_get_imag_self_energy(PyObject *self, PyObject *args);
static PyObject * py_get_imag_self_energy_at_bands(PyObject *self,
						   PyObject *args);
static PyObject * py_set_phonon_triplets(PyObject *self, PyObject *args);
static PyObject * py_get_phonon(PyObject *self, PyObject *args);
static PyObject * py_distribute_fc3(PyObject *self, PyObject *args);
static PyObject * py_phonopy_zheev(PyObject *self, PyObject *args);

static int distribute_fc3(double *fc3,
			  const int third_atom,
			  const int third_atom_rot,
			  const double *positions,
			  const int num_atom,
			  const int * rot,
			  const double *rot_cartesian,
			  const double *trans,
			  const double symprec);
static int get_atom_by_symmetry(const int atom_number,
				const int num_atom,
				const double *positions,
				const int *rot,
				const double *trans,
				const double symprec);
static void tensor3_roation(double *fc3,
			    const int third_atom,
			    const int third_atom_rot,
			    const int atom_i,
			    const int atom_j,
			    const int atom_rot_i,
			    const int atom_rot_j,
			    const int num_atom,
			    const double *rot_cartesian);
static double tensor3_rotation_elem(double tensor[3][3][3],
				    const double *r,
				    const int l,
				    const int m,
				    const int n);
static int nint(const double a);

static PyMethodDef functions[] = {
  {"joint_dos", py_get_jointDOS, METH_VARARGS, "Calculate joint density of states"},
  {"interaction", py_get_interaction, METH_VARARGS, "Interaction of triplets"},
  {"imag_self_energy", py_get_imag_self_energy, METH_VARARGS, "Imaginary part of self energy"},
  {"imag_self_energy_at_bands", py_get_imag_self_energy_at_bands, METH_VARARGS, "Imaginary part of self energy at phonon frequencies of bands"},
  {"phonon_triplets", py_set_phonon_triplets, METH_VARARGS, "Set phonon triplets"},
  {"phonon", py_get_phonon, METH_VARARGS, "Get phonon"},
  {"distribute_fc3", py_distribute_fc3, METH_VARARGS, "Distribute least fc3 to full fc3"},
  {"zheev", py_phonopy_zheev, METH_VARARGS, "Lapack zheev wrapper"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_phono3py(void)
{
  Py_InitModule3("_phono3py", functions, "C-extension for phono3py\n\n...\n");
  return;
}

static PyObject * py_set_phonon_triplets(PyObject *self, PyObject *args)
{
  PyArrayObject* frequencies;
  PyArrayObject* eigenvectors;
  PyArrayObject* phonon_done_py;
  PyArrayObject* grid_point_triplets;
  PyArrayObject* grid_address_py;
  PyArrayObject* mesh_py;
  PyArrayObject* shortest_vectors_fc2;
  PyArrayObject* multiplicity_fc2;
  PyArrayObject* fc2_py;
  PyArrayObject* atomic_masses_fc2;
  PyArrayObject* p2s_map_fc2;
  PyArrayObject* s2p_map_fc2;
  PyArrayObject* reciprocal_lattice;
  PyArrayObject* born_effective_charge;
  PyArrayObject* q_direction;
  PyArrayObject* dielectric_constant;
  double nac_factor, unit_conversion_factor;
  char uplo;

  if (!PyArg_ParseTuple(args, "OOOOOOOOOOOOdOOOOdc",
			&frequencies,
			&eigenvectors,
			&phonon_done_py,
			&grid_point_triplets,
			&grid_address_py,
			&mesh_py,
			&fc2_py,
			&shortest_vectors_fc2,
			&multiplicity_fc2,
			&atomic_masses_fc2,
			&p2s_map_fc2,
			&s2p_map_fc2,
			&unit_conversion_factor,
			&born_effective_charge,
			&dielectric_constant,
			&reciprocal_lattice,
			&q_direction,
			&nac_factor,
			&uplo)) {
    return NULL;
  }

  double* born;
  double* dielectric;
  double *q_dir;
  Darray* freqs = convert_to_darray(frequencies);
  /* npy_cdouble and lapack_complex_double may not be compatible. */
  /* So eigenvectors should not be used in Python side */
  Carray* eigvecs = convert_to_carray(eigenvectors);
  char* phonon_done = (char*)phonon_done_py->data;
  Iarray* triplets = convert_to_iarray(grid_point_triplets);
  Iarray* grid_address = convert_to_iarray(grid_address_py);
  const int* mesh = (int*)mesh_py->data;
  Darray* fc2 = convert_to_darray(fc2_py);
  Darray* svecs_fc2 = convert_to_darray(shortest_vectors_fc2);
  Iarray* multi_fc2 = convert_to_iarray(multiplicity_fc2);
  const double* masses_fc2 = (double*)atomic_masses_fc2->data;
  const int* p2s_fc2 = (int*)p2s_map_fc2->data;
  const int* s2p_fc2 = (int*)s2p_map_fc2->data;
  const double* rec_lat = (double*)reciprocal_lattice->data;
  if ((PyObject*)born_effective_charge == Py_None) {
    born = NULL;
  } else {
    born = (double*)born_effective_charge->data;
  }
  if ((PyObject*)dielectric_constant == Py_None) {
    dielectric = NULL;
  } else {
    dielectric = (double*)dielectric_constant->data;
  }
  if ((PyObject*)q_direction == Py_None) {
    q_dir = NULL;
  } else {
    q_dir = (double*)q_direction->data;
  }

  set_phonon_triplets(freqs,
		      eigvecs,
		      phonon_done,
		      triplets,
		      grid_address,
		      mesh,
		      fc2,
		      svecs_fc2,
		      multi_fc2,
		      masses_fc2,
		      p2s_fc2,
		      s2p_fc2,
		      unit_conversion_factor,
		      born,
		      dielectric,
		      rec_lat,
		      q_dir,
		      nac_factor,
		      uplo);

  free(freqs);
  free(eigvecs);
  free(triplets);
  free(grid_address);
  free(fc2);
  free(svecs_fc2);
  free(multi_fc2);
  
  Py_RETURN_NONE;
}

static PyObject * py_get_phonon(PyObject *self, PyObject *args)
{
  PyArrayObject* frequencies_py;
  PyArrayObject* eigenvectors_py;
  PyArrayObject* q_py;
  PyArrayObject* shortest_vectors_py;
  PyArrayObject* multiplicity_py;
  PyArrayObject* fc2_py;
  PyArrayObject* atomic_masses_py;
  PyArrayObject* p2s_map_py;
  PyArrayObject* s2p_map_py;
  PyArrayObject* reciprocal_lattice_py;
  PyArrayObject* born_effective_charge_py;
  PyArrayObject* q_direction_py;
  PyArrayObject* dielectric_constant_py;
  double nac_factor, unit_conversion_factor;
  char uplo;

  if (!PyArg_ParseTuple(args, "OOOOOOOOOdOOOOdc",
			&frequencies_py,
			&eigenvectors_py,
			&q_py,
			&fc2_py,
			&shortest_vectors_py,
			&multiplicity_py,
			&atomic_masses_py,
			&p2s_map_py,
			&s2p_map_py,
			&unit_conversion_factor,
			&born_effective_charge_py,
			&dielectric_constant_py,
			&reciprocal_lattice_py,
			&q_direction_py,
			&nac_factor,
			&uplo)) {
    return NULL;
  }

  double* born;
  double* dielectric;
  double *q_dir;
  double* freqs = (double*)frequencies_py->data;
  /* npy_cdouble and lapack_complex_double may not be compatible. */
  /* So eigenvectors should not be used in Python side */
  lapack_complex_double* eigvecs =
    (lapack_complex_double*)eigenvectors_py->data;
  const double* q = (double*) q_py->data;
  Darray* fc2 = convert_to_darray(fc2_py);
  Darray* svecs = convert_to_darray(shortest_vectors_py);
  Iarray* multi = convert_to_iarray(multiplicity_py);
  const double* masses = (double*)atomic_masses_py->data;
  const int* p2s = (int*)p2s_map_py->data;
  const int* s2p = (int*)s2p_map_py->data;
  const double* rec_lat = (double*)reciprocal_lattice_py->data;

  if ((PyObject*)born_effective_charge_py == Py_None) {
    born = NULL;
  } else {
    born = (double*)born_effective_charge_py->data;
  }
  if ((PyObject*)dielectric_constant_py == Py_None) {
    dielectric = NULL;
  } else {
    dielectric = (double*)dielectric_constant_py->data;
  }
  if ((PyObject*)q_direction_py == Py_None) {
    q_dir = NULL;
  } else {
    q_dir = (double*)q_direction_py->data;
  }

  get_phonons(eigvecs,
	      freqs,
	      q,
	      fc2,
	      masses,
	      p2s,
	      s2p,
	      multi,
	      svecs,
	      born,
	      dielectric,
	      rec_lat,
	      q_dir,
	      nac_factor,
	      unit_conversion_factor,
	      uplo);

  Py_RETURN_NONE;
}


static PyObject * py_get_interaction(PyObject *self, PyObject *args)
{
  PyArrayObject* fc3_normal_squared_py;
  PyArrayObject* frequencies;
  PyArrayObject* eigenvectors;
  PyArrayObject* grid_point_triplets;
  PyArrayObject* grid_address_py;
  PyArrayObject* mesh_py;
  PyArrayObject* shortest_vectors;
  PyArrayObject* multiplicity;
  PyArrayObject* fc3_py;
  PyArrayObject* atomic_masses;
  PyArrayObject* p2s_map;
  PyArrayObject* s2p_map;
  PyArrayObject* band_indicies_py;

  if (!PyArg_ParseTuple(args, "OOOOOOOOOOOOO",
			&fc3_normal_squared_py,
			&frequencies,
			&eigenvectors,
			&grid_point_triplets,
			&grid_address_py,
			&mesh_py,
			&fc3_py,
			&shortest_vectors,
			&multiplicity,
			&atomic_masses,
			&p2s_map,
			&s2p_map,
			&band_indicies_py)) {
    return NULL;
  }


  Darray* fc3_normal_squared = convert_to_darray(fc3_normal_squared_py);
  Darray* freqs = convert_to_darray(frequencies);
  /* npy_cdouble and lapack_complex_double may not be compatible. */
  /* So eigenvectors should not be used in Python side */
  Carray* eigvecs = convert_to_carray(eigenvectors);
  Iarray* triplets = convert_to_iarray(grid_point_triplets);
  Iarray* grid_address = convert_to_iarray(grid_address_py);
  const int* mesh = (int*)mesh_py->data;
  Darray* fc3 = convert_to_darray(fc3_py);
  Darray* svecs = convert_to_darray(shortest_vectors);
  Iarray* multi = convert_to_iarray(multiplicity);
  const double* masses = (double*)atomic_masses->data;
  const int* p2s = (int*)p2s_map->data;
  const int* s2p = (int*)s2p_map->data;
  const int* band_indicies = (int*)band_indicies_py->data;

  get_interaction(fc3_normal_squared,
		  freqs,
		  eigvecs,
		  triplets,
		  grid_address,
		  mesh,
		  fc3,
		  svecs,
		  multi,
		  masses,
		  p2s,
		  s2p,
		  band_indicies);

  free(fc3_normal_squared);
  free(freqs);
  free(eigvecs);
  free(triplets);
  free(grid_address);
  free(fc3);
  free(svecs);
  free(multi);
  
  Py_RETURN_NONE;
}

static PyObject * py_get_imag_self_energy(PyObject *self, PyObject *args)
{
  PyArrayObject* gamma_py;
  PyArrayObject* fc3_normal_squared_py;
  PyArrayObject* frequencies_py;
  PyArrayObject* grid_point_triplets_py;
  PyArrayObject* triplet_weights_py;
  double sigma, unit_conversion_factor, temperature, fpoint;

  if (!PyArg_ParseTuple(args, "OOOOOdddd",
			&gamma_py,
			&fc3_normal_squared_py,
			&grid_point_triplets_py,
			&triplet_weights_py,
			&frequencies_py,
			&fpoint,
			&temperature,
			&sigma,
			&unit_conversion_factor)) {
    return NULL;
  }


  Darray* fc3_normal_squared = convert_to_darray(fc3_normal_squared_py);
  double* gamma = (double*)gamma_py->data;
  const double* frequencies = (double*)frequencies_py->data;
  const int* grid_point_triplets = (int*)grid_point_triplets_py->data;
  const int* triplet_weights = (int*)triplet_weights_py->data;

  get_imag_self_energy(gamma,
		       fc3_normal_squared,
		       fpoint,
		       frequencies,
		       grid_point_triplets,
		       triplet_weights,
		       sigma,
		       temperature,
		       unit_conversion_factor);

  free(fc3_normal_squared);
  
  Py_RETURN_NONE;
}

static PyObject * py_get_imag_self_energy_at_bands(PyObject *self,
						   PyObject *args)
{
  PyArrayObject* gamma_py;
  PyArrayObject* fc3_normal_squared_py;
  PyArrayObject* frequencies_py;
  PyArrayObject* grid_point_triplets_py;
  PyArrayObject* triplet_weights_py;
  PyArrayObject* band_indices_py;
  double sigma, unit_conversion_factor, temperature;

  if (!PyArg_ParseTuple(args, "OOOOOOddd",
			&gamma_py,
			&fc3_normal_squared_py,
			&grid_point_triplets_py,
			&triplet_weights_py,
			&frequencies_py,
			&band_indices_py,
			&temperature,
			&sigma,
			&unit_conversion_factor)) {
    return NULL;
  }


  Darray* fc3_normal_squared = convert_to_darray(fc3_normal_squared_py);
  double* gamma = (double*)gamma_py->data;
  const double* frequencies = (double*)frequencies_py->data;
  const int* band_indices = (int*)band_indices_py->data;
  const int* grid_point_triplets = (int*)grid_point_triplets_py->data;
  const int* triplet_weights = (int*)triplet_weights_py->data;

  get_imag_self_energy_at_bands(gamma,
				fc3_normal_squared,
				band_indices,
				frequencies,
				grid_point_triplets,
				triplet_weights,
				sigma,
				temperature,
				unit_conversion_factor);

  free(fc3_normal_squared);
  
  Py_RETURN_NONE;
}

static PyObject * py_get_jointDOS(PyObject *self, PyObject *args)
{
  PyArrayObject* jointdos;
  PyArrayObject* omegas;
  PyArrayObject* weights;
  PyArrayObject* frequencies;
  double sigma;

  if (!PyArg_ParseTuple(args, "OOOOd",
			&jointdos,
			&omegas,
			&weights,
			&frequencies,
			&sigma)) {
    return NULL;
  }
  
  double* jdos = (double*)jointdos->data;
  const double* o = (double*)omegas->data;
  const int* w = (int*)weights->data;
  const double* f = (double*)frequencies->data;
  const int num_band = (int)frequencies->dimensions[2];
  const int num_omega = (int)omegas->dimensions[0];
  const int num_triplet = (int)weights->dimensions[0];

  get_jointDOS(jdos,
	       num_omega,
	       num_triplet,
	       num_band,
	       o,
	       f,
	       w,
	       sigma);
  
  Py_RETURN_NONE;
}

static PyObject * py_distribute_fc3(PyObject *self, PyObject *args)
{
  PyArrayObject* force_constants_third;
  int third_atom, third_atom_rot;
  PyArrayObject* positions;
  PyArrayObject* rotation;
  PyArrayObject* rotation_cart_inv;
  PyArrayObject* translation;
  double symprec;

  if (!PyArg_ParseTuple(args, "OiiOOOOd",
			&force_constants_third,
			&third_atom,
			&third_atom_rot,
			&positions,
			&rotation,
			&rotation_cart_inv,
			&translation,
			&symprec))
    return NULL;

  double* fc3 = (double*)force_constants_third->data;
  const double* pos = (double*)positions->data;
  const int* rot = (int*)rotation->data;
  const double* rot_cart_inv = (double*)rotation_cart_inv->data;
  const double* trans = (double*)translation->data;
  const int num_atom = (int)positions->dimensions[0];

  return PyInt_FromLong((long) distribute_fc3(fc3,
					      third_atom,
					      third_atom_rot,
					      pos,
					      num_atom,
					      rot,
					      rot_cart_inv,
					      trans,
					      symprec));
}

static PyObject * py_phonopy_zheev(PyObject *self, PyObject *args)
{
  PyArrayObject* dynamical_matrix;
  PyArrayObject* eigenvalues;

  if (!PyArg_ParseTuple(args, "OO",
			&dynamical_matrix,
			&eigenvalues)) {
    return NULL;
  }

  const int dimension = (int)dynamical_matrix->dimensions[0];
  npy_cdouble *dynmat = (npy_cdouble*)dynamical_matrix->data;
  double *eigvals = (double*)eigenvalues->data;

  lapack_complex_double *a;
  int i, info;

  a = (lapack_complex_double*) malloc(sizeof(lapack_complex_double) *
				      dimension * dimension);
  for (i = 0; i < dimension * dimension; i++) {
    a[i] = lapack_make_complex_double(dynmat[i].real, dynmat[i].imag);
  }

  info = phonopy_zheev(eigvals, a, dimension, 'L');

  for (i = 0; i < dimension * dimension; i++) {
    dynmat[i].real = lapack_complex_double_real(a[i]);
    dynmat[i].imag = lapack_complex_double_imag(a[i]);
  }

  free(a);
  
  return PyInt_FromLong((long) info);
}


static int distribute_fc3(double *fc3,
			  const int third_atom,
			  const int third_atom_rot,
			  const double *positions,
			  const int num_atom,
			  const int *rot,
			  const double *rot_cart_inv,
			  const double *trans,
			  const double symprec)
{
  int i, j, atom_rot_i, atom_rot_j;
  
  for (i = 0; i < num_atom; i++) {
    atom_rot_i =
      get_atom_by_symmetry(i, num_atom, positions, rot, trans, symprec);

    if (atom_rot_i < 0) {
      fprintf(stderr, "phono3c: Unexpected behavior in distribute_fc3.\n");
      fprintf(stderr, "phono3c: atom_i %d\n", i);
      return 0;
    }

    for (j = 0; j < num_atom; j++) {
      atom_rot_j =
	get_atom_by_symmetry(j, num_atom, positions, rot, trans, symprec);

      if (atom_rot_j < 0) {
	fprintf(stderr, "phono3c: Unexpected behavior in distribute_fc3.\n");
	return 0;
      }

      tensor3_roation(fc3,
		      third_atom,
		      third_atom_rot,
		      i,
		      j,
		      atom_rot_i,
		      atom_rot_j,
		      num_atom,
		      rot_cart_inv);
    }
  }
  return 1;
}

static int get_atom_by_symmetry(const int atom_number,
				const int num_atom,
				const double *positions,
				const int *rot,
				const double *trans,
				const double symprec)
{
  int i, j, found;
  double rot_pos[3], diff[3];
  
  for (i = 0; i < 3; i++) {
    rot_pos[i] = trans[i];
    for (j = 0; j < 3; j++) {
      rot_pos[i] += rot[i * 3 + j] * positions[atom_number * 3 + j];
    }
  }

  for (i = 0; i < num_atom; i++) {
    found = 1;
    for (j = 0; j < 3; j++) {
      diff[j] = positions[i * 3 + j] - rot_pos[j];
      diff[j] -= nint(diff[j]);
      if (fabs(diff[j]) > symprec) {
	found = 0;
	break;
      }
    }
    if (found) {
      return i;
    }
  }
  /* Not found */
  return -1;
}

static void tensor3_roation(double *fc3,
			    const int third_atom,
			    const int third_atom_rot,
			    const int atom_i,
			    const int atom_j,
			    const int atom_rot_i,
			    const int atom_rot_j,
			    const int num_atom,
			    const double *rot_cart_inv)
{
  int i, j, k;
  double tensor[3][3][3];

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      for (k = 0; k < 3; k++) {
	tensor[i][j][k] = fc3[27 * num_atom * num_atom * third_atom_rot +
			      27 * num_atom * atom_rot_i +
			      27 * atom_rot_j +
			      9 * i + 3 * j + k];
      }
    }
  }

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      for (k = 0; k < 3; k++) {
	fc3[27 * num_atom * num_atom * third_atom +
	    27 * num_atom * atom_i +
	    27 * atom_j +
	    9 * i + 3 * j + k] = 
	  tensor3_rotation_elem(tensor, rot_cart_inv, i, j, k);
      }
    }
  }
}

static double tensor3_rotation_elem(double tensor[3][3][3],
				    const double *r,
				    const int l,
				    const int m,
				    const int n) 
{
  int i, j, k;
  double sum;

  sum = 0.0;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      for (k = 0; k < 3; k++) {
	sum += r[l * 3 + i] * r[m * 3 + j] * r[n * 3 + k] * tensor[i][j][k];
      }
    }
  }
  return sum;
}


static int nint(const double a)
{
  if (a < 0.0)
    return (int) (a - 0.5);
  else
    return (int) (a + 0.5);
}

