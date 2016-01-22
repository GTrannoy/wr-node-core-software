/**
 * Copyright (C) 2016 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#include <stdio.h>

#include "wrtd-ut.h"
#include "CuTest.h"

CuSuite* CuGetSuite();
CuSuite* CuStringGetSuite();

void RunAllTests(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();


	CuSuiteAddSuite(suite, wrtd_ut_in_suite_get());
	CuSuiteAddSuite(suite, wrtd_ut_out_suite_get());
	CuSuiteAddSuite(suite, wrtd_ut_cmm_suite_get());
	CuSuiteAddSuite(suite, wrtd_ut_op_suite_get());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
}

int main(int argc, char *argv[])
{
	RunAllTests();

	return 0;
}
