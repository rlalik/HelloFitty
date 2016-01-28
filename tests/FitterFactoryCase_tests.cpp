#include <cppunit/extensions/HelperMacros.h>

#include <FitterFactory.h>

class FitterFactoryCase_tests : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( class FitterFactoryCase_tests );
	CPPUNIT_TEST( MyTest );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();

protected:
	void MyTest();
};

CPPUNIT_TEST_SUITE_REGISTRATION( FitterFactoryCase_tests );

void FitterFactoryCase_tests::setUp()
{
}

void FitterFactoryCase_tests::MyTest()
{
	FitterFactory ff;

	std::string p1 = "pref1_";
	std::string s1 = "_suff1";

	std::string tn1 = "test_name";
	std::string tn2 = p1 + tn1 + s1;
	std::string tn3 = "replaced";
	std::string tn4 = p1 + tn3 + s1;

	ff.setPrefix(p1);
	ff.setSuffix(s1);

	CPPUNIT_ASSERT_EQUAL(tn1, ff.format_name(tn1));
	CPPUNIT_ASSERT_EQUAL(tn2, ff.format_name(tn2));

	ff.setPrefixManipulator(FitterFactory::PS_APPEND);
	ff.setSuffixManipulator(FitterFactory::PS_IGNORE);
	CPPUNIT_ASSERT_EQUAL(p1 + tn1, ff.format_name(tn1));

	ff.setPrefixManipulator(FitterFactory::PS_IGNORE);
	ff.setSuffixManipulator(FitterFactory::PS_APPEND);
	CPPUNIT_ASSERT_EQUAL(tn1 + s1, ff.format_name(tn1));

	ff.setPrefixManipulator(FitterFactory::PS_APPEND);
	ff.setSuffixManipulator(FitterFactory::PS_APPEND);
	CPPUNIT_ASSERT_EQUAL(p1 + tn1 + s1, ff.format_name(tn1));

	ff.setPrefixManipulator(FitterFactory::PS_SUBSTRACT);
	ff.setSuffixManipulator(FitterFactory::PS_IGNORE);
	CPPUNIT_ASSERT_EQUAL(tn1 + s1, ff.format_name(tn2));

	ff.setPrefixManipulator(FitterFactory::PS_IGNORE);
	ff.setSuffixManipulator(FitterFactory::PS_SUBSTRACT);
	CPPUNIT_ASSERT_EQUAL(p1 + tn1, ff.format_name(tn2));

	ff.setPrefixManipulator(FitterFactory::PS_SUBSTRACT);
	ff.setSuffixManipulator(FitterFactory::PS_SUBSTRACT);
	CPPUNIT_ASSERT_EQUAL(tn1, ff.format_name(tn2));

	ff.setReplacement(tn1, tn3);
	CPPUNIT_ASSERT_EQUAL(tn3, ff.format_name(tn2));
}
