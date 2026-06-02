// @ts-check
const { defineConfig, devices } = require("@playwright/test");

module.exports = defineConfig({
  testDir: "./tests",
  timeout: 30_000,
  expect: {
    timeout: 5_000,
  },
  use: {
    baseURL: "http://127.0.0.1:3000",
    trace: "on-first-retry",
  },
  webServer: {
    command: "node proxy/server.js",
    url: "http://127.0.0.1:3000",
    reuseExistingServer: true,
    timeout: 10_000,
  },
  projects: [
    {
      name: "chrome-desktop",
      use: {
        ...devices["Desktop Chrome"],
        channel: "chrome",
        viewport: { width: 1440, height: 900 },
      },
    },
    {
      name: "chrome-mobile",
      use: {
        ...devices["Pixel 5"],
        channel: "chrome",
      },
    },
  ],
});
