#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestPath.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/plugin/DynamicLibraryManagerException.h>
#include <cppunit/plugin/Parameters.h>
#include <cppunit/plugin/PlugInManager.h>
#include <cppunit/plugin/TestPlugIn.h>
#include <iostream>
#include <fstream>
#include "CommandLineParser.h"


/* Notes:

  Memory allocated by test plug-in must be freed before unloading the test plug-in.
  That is the reason why the XmlOutputter is explicitely destroyed.
 */


/*! Runs the specified tests located in the root suite.
 * \param parser Command line parser.
 * \return \c true if the run succeed, \c false if a test failed or if a test
 *         path was not resolved.
 */
bool 
runTests( const CommandLineParser &parser )
{
  bool wasSuccessful = false;
  CppUnit::PlugInManager plugInManager;

  // The following scope is used to explicitely free all memory allocated before
  // unload the test plug-ins (uppon plugInManager destruction).
  {
    CppUnit::TestResult controller;
    CppUnit::TestResultCollector result;
    controller.addListener( &result );        

    // Set up outputters
    std::ostream *stream = &std::cerr;
    if ( parser.useCoutStream() )
      stream = &std::cout;

    std::ostream *xmlStream = stream;
    if ( !parser.getXmlFileName().empty() )
      xmlStream = new std::ofstream( parser.getXmlFileName().c_str() );

    CppUnit::XmlOutputter xmlOutputter( &result, *xmlStream, parser.getEncoding() );
    xmlOutputter.setStyleSheet( parser.getXmlStyleSheet() );
    CppUnit::TextOutputter textOutputter( &result, *stream );
    CppUnit::CompilerOutputter compilerOutputter( &result, *stream );

    // Set up test listeners
    CppUnit::BriefTestProgressListener briefListener;
    CppUnit::TextTestProgressListener dotListener;
    if ( parser.useBriefTestProgress() )
      controller.addListener( &briefListener );
    else if ( !parser.noTestProgress() )
      controller.addListener( &dotListener );

    // Set up plug-ins
    for ( int index =0; index < parser.getPlugInCount(); ++index )
    {
      CommandLinePlugInInfo plugIn = parser.getPlugInAt( index );
      plugInManager.load( plugIn.m_fileName, plugIn.m_parameters );
    }

    // Registers plug-in specific TestListener (global setUp/tearDown, custom TestListener...)
    plugInManager.addListener( &controller );

    // Adds the default registry suite
    CppUnit::TestRunner runner;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );

    // Runs the specified test
    try
    {
      runner.run( controller, parser.getTestPath() );
      wasSuccessful = result.wasSuccessful();
    }
    catch ( std::invalid_argument & )
    {
      std::cerr  <<  "Failed to resolve test path: "  
                 <<  parser.getTestPath() 
                 <<  std::endl;
    }

    // Removes plug-in specific TestListener (not really needed but...)
    plugInManager.removeListener( &controller );

    // write using outputters
    if ( parser.useCompilerOutputter() )
      compilerOutputter.write();

    if ( parser.useTextOutputter() )
      textOutputter.write();

    if ( parser.useXmlOutputter() )
    {
      plugInManager.addXmlOutputterHooks( &xmlOutputter );
      xmlOutputter.write();
      plugInManager.removeXmlOutputterHooks();
    }

    if ( !parser.getXmlFileName().empty() )
      delete xmlStream;
  }

  return wasSuccessful;
}


void
printShortUsage( const std::string &applicationName )
{
  std::cout  << "Usage:"  <<  std::endl
             << applicationName  <<  " [-c -b -n -t -o -w] [-x xml-filename]"
             "[-s stylesheet] [-e encoding] plug-in[=parameters] [plug-in...] [:testPath]"
             << std::endl  <<  std::endl;
}


void
printUsage( const std::string &applicationName )
{
  printShortUsage( applicationName );
  std::cout  <<
"-c --compiler\n"
"	Use CompilerOutputter\n"
"-x --xml [filename]\n"
"	Use XmlOutputter (if filename is omitted, then output to cout or\n"
"	cerr.\n"
"-s --xsl stylesheet\n"
"	XML style sheet for XML Outputter\n"
"-e --encoding encoding\n"
"	XML file encoding (UTF8, shift_jis, ISO-8859-1...)\n"
"-b --brief-progress\n"
"	Use BriefTestProgressListener (default is TextTestProgressListener)\n"
"-n --no-progress\n"
"	Show no test progress (disable default TextTestProgressListener)\n"
"-t --text\n"
"	Use TextOutputter\n"
"-o --cout\n"
"	Ouputters output to cout instead of the default cerr.\n"
"-w --wait\n"
"	Wait for the user to press a return before exit.\n"
"filename[=\"options\"]\n"
"	Many filenames can be specified. They are the name of the \n"
"	test plug-ins to load. Optional plug-ins parameters can be \n"
"	specified after the filename by adding '='.\n"
"[:testpath]\n"
"	Optional. Only one test path can be specified. It must \n"
"	be prefixed with ':'. See TestPath constructor for syntax.\n"
"\n"
"'parameters' (test plug-in or XML filename, test path...) may contains \n"
"spaces if double quoted. Quote may be escaped with \".\n"
"\n"
"Some examples of command lines:\n"
"\n"
"DllPlugInTesterd_dll.exe -b -x tests.xml -c simple_plugind.dll CppUnitTestPlugInd.dll\n"
"\n"
" Will load 2 tests plug-ins (available in lib/), use the brief test\n"
"progress, output the result in XML in file tests.xml and also\n"
"output the result using the compiler outputter.\n"
"\n"
"DllPlugInTesterd_dll.exe ClockerPlugInd.dll=\"flat\" -n CppUnitTestPlugInd.dll\n"
"\n"
" Will load the 2 test plug-ins, and pass the parameter string \"flat\"\n"
"to the Clocker plug-in, disable test progress.\n"
  << std::endl;

}


/*! Main
 * 
 * Usage: 
 *
 * DllPlugInTester.exe dll-filename1 [dll-filename2 [dll-filename3 ...]] [:testpath]
 *
 * <em>dll-filename</em> must be the name of the DLL. If the DLL use some other DLL, they
 * should be in the path or in the same directory as the DLL. The DLL must export
 * a function named "GetTestPlugInInterface" with the signature
 * GetTestPlugInInterfaceFunction. Both are defined in:
 * \code
 * #include <msvc6/testrunner/TestPlugInInterface.h>
 * \endcode.
 *
 * See examples/msvc6/TestPlugIn for an example of post-build testing.
 *
 * If no test path is specified, they all the test of the suite returned by the DLL
 * are run. If a test path is specified, then only the specified test is run. The test
 * path must be prefixed by ':'.
 *
 * Test paths are resolved using Test::resolveTestPath() on the suite returned by
 * TestFactoryRegistry::getRegistry().makeTest();
 *
 * If all test succeed and no error happen then the application exit with code 0.
 * If any error occurs (failed to load dll, failed to resolve test paths) or a 
 * test fail, the application exit with code 1. If the application failed to
 * parse the command line, it exits with code 2.
 */
int 
main( int argc, 
      char *argv[] )
{
  const int successReturnCode = 0;
  const int failureReturnCode = 1;
  const int badCommadLineReturnCode = 2;

  // check command line
  std::string applicationName( argv[0] );
  if ( argc < 2 )
  {
    printUsage( applicationName );
    return badCommadLineReturnCode;
  }

  CommandLineParser parser( argc, argv );
  try
  {
    parser.parse();
  }
  catch ( CommandLineParserException &e )
  {
    std::cerr  <<  "Error while parsing command line: "  <<  e.what()  
               << std::endl << std::endl;
    printShortUsage( applicationName );
    return badCommadLineReturnCode;
  }

  bool wasSuccessful = false;
  try
  {
    wasSuccessful = runTests( parser );
  }
  catch ( CppUnit::DynamicLibraryManagerException &e )
  {
    std::cerr  << "Failed to load test plug-in:"  <<  std::endl
               << e.what() << std::endl;
  }

  if ( parser.waitBeforeExit() )
  {
    std::cout << "Please press <RETURN> to exit" << std::endl;
    std::cin.get();
  }

  return wasSuccessful ? successReturnCode : failureReturnCode;
}

