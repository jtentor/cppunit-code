#include "cppunit/TestSuite.h"
#include "cppunit/TestResult.h"
#ifdef USE_TYPEINFO
#include "TypeInfoHelper.h"
#endif // USE_TYPEINFO

namespace CppUnit {

/// Deletes all tests in the suite.
void TestSuite::deleteContents ()
{
  for (std::vector<Test *>::iterator it = m_tests.begin ();
       it != m_tests.end ();
       ++it)
    delete *it;
  m_tests.clear();
}


/// Runs the tests and collects their result in a TestResult.
void TestSuite::run (TestResult *result)
{
    for (std::vector<Test *>::iterator it = m_tests.begin ();
            it != m_tests.end ();
            ++it) {
        if (result->shouldStop ())
            break;

        Test *test = *it;
        test->run (result);
    }

}


/// Counts the number of test cases that will be run by this test.
int TestSuite::countTestCases () const
{
    int count = 0;

    for (std::vector<Test *>::const_iterator it = m_tests.begin ();
            it != m_tests.end ();
            ++it)
        count += (*it)->countTestCases ();

    return count;

}



/// Default constructor
TestSuite::TestSuite (std::string name)
  : m_name (name)
{
}

#ifdef USE_TYPEINFO
/** Constructs a test suite named after the specified type_info.
 * \param info type_info used to name the suite. The 'class' prefix
 *             is stripped from the name.
 */
TestSuite::TestSuite(const std::type_info &info ) :
  m_name( TypeInfoHelper::getClassName( info ) )
{
}
#endif // USE_TYPEINFO


/// Destructor
TestSuite::~TestSuite ()
{ 
  deleteContents (); 
}


/// Adds a test to the suite. 
void 
  TestSuite::addTest (Test *test)
{ 
  m_tests.push_back (test); 
}


/// Returns a string representation of the test suite.
std::string 
  TestSuite::toString () const
{ 
  return "suite " + getName(); 
}

/// Returns the name of the test suite.
std::string 
  TestSuite::getName () const
{ 
  return m_name; 
}

const std::vector<Test *>& 
  TestSuite::getTests () const
{
  return m_tests;
}

} // namespace CppUnit

