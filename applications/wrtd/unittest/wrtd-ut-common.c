#include "wrtd-ut.h"
#include "CuTest.h"

#include <libwrnc.h>
#include <libwrtd.h>

static void test_open_close(CuTest *tc)
{
	struct wrtd_node *wrtd;

	wrtd = wrtd_open_by_lun(0);

	CuAssertTrue(tc, !!wrtd);
	CuAssertTrue(tc, wrtd_version_is_valid(wrtd));

	wrtd_close(wrtd);
}

CuSuite *wrtd_ut_cmm_suite_get(void)
{
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_open_close);

	return suite;
}
