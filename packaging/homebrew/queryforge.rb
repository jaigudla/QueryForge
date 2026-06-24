class Queryforge < Formula
  desc "Benchmark in-memory query strategies for C++ developers"
  homepage "https://github.com/your-org/QueryForge"
  url "https://github.com/your-org/QueryForge/releases/download/v1.0.0/queryforge-macos.tar.gz"
  sha256 "REPLACE_WITH_RELEASE_SHA256"
  license "MIT"

  def install
    bin.install "queryforge"
  end

  test do
    assert_match "1.0.0", shell_output("#{bin}/queryforge --version")
  end
end
