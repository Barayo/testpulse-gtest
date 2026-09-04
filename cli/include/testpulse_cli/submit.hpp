#pragma once

#include <testpulse_cli/config.hpp>
#include <testpulse_cli/http_client.hpp>

#include <ostream>
#include <string>

namespace testpulse_cli {

// Reads the JUnit XML report at reportPath plus any attachments under
// config.dir, and either submits them (POST .../imports) or, if
// config.dryRun, previews matches via a read-only GET .../cases. Returns
// the process exit code: 0 for success (including a 207 with unmatched
// cases when failOnUnmatched is false), non-zero otherwise.
int RunSubmit(const Config& config, const std::string& reportPath, HttpClient& client,
              std::ostream& out, std::ostream& err);

}  // namespace testpulse_cli
