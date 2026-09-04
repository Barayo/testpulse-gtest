#include <gtest/gtest.h>
#include <testpulse_cli/report_reader.hpp>

using testpulse_cli::ExtractDeclaredCaseKeys;

TEST(ReportReaderTest, ExtractsASingleCaseKey) {
    std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="LoginTest">
    <testcase name="Succeeds" classname="LoginTest">
      <properties>
        <property name="testpulse_case_key" value="LOGIN-42"/>
      </properties>
    </testcase>
  </testsuite>
</testsuites>)";
    EXPECT_EQ(ExtractDeclaredCaseKeys(xml), std::vector<std::string>{"LOGIN-42"});
}

TEST(ReportReaderTest, ExtractsMultipleCaseKeysInDocumentOrder) {
    std::string xml = R"(<testsuites>
  <testsuite name="LoginTest">
    <testcase name="A"><properties><property name="testpulse_case_key" value="LOGIN-1"/></properties></testcase>
    <testcase name="B"><properties><property name="testpulse_case_key" value="LOGIN-2"/></properties></testcase>
  </testsuite>
</testsuites>)";
    std::vector<std::string> expected = {"LOGIN-1", "LOGIN-2"};
    EXPECT_EQ(ExtractDeclaredCaseKeys(xml), expected);
}

TEST(ReportReaderTest, IgnoresOtherProperties) {
    std::string xml = R"(<testsuites>
  <testsuite name="LoginTest">
    <testcase name="A">
      <properties>
        <property name="testpulse_platform" value="linux"/>
        <property name="testpulse_case_key" value="LOGIN-1"/>
      </properties>
    </testcase>
  </testsuite>
</testsuites>)";
    EXPECT_EQ(ExtractDeclaredCaseKeys(xml), std::vector<std::string>{"LOGIN-1"});
}

TEST(ReportReaderTest, NoPropertiesAtAllReturnsEmpty) {
    std::string xml = R"(<testsuites><testsuite name="X"><testcase name="A"/></testsuite></testsuites>)";
    EXPECT_TRUE(ExtractDeclaredCaseKeys(xml).empty());
}
