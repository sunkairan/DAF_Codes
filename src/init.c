#include "Utilities.h"
#include "FiniteField.h"

#include "ff.h"
#include "vand.h"
#include "bats.h"

int bats_init()
{
	ff_init();
	FF.setOrder(8);

	
	init_vandermond_matrix(16);

	return 0;
}
