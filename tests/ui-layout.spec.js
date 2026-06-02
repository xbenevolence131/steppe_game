const { test, expect } = require("@playwright/test");

async function openPlayMode(page) {
  await page.goto("/");
  await page.getByRole("button", { name: "Play" }).click();
  await expect(page.locator(".unit-roster-item")).toHaveCount(4);
}

test("scenario controls sit above the shared map", async ({ page }) => {
  await page.goto("/");
  await page.getByRole("button", { name: "Scenario Edit" }).click();

  await expect(page.locator("#scenario-controls")).toBeVisible();
  await expect(page.locator("#play-controls")).toBeHidden();
  await expect(page.locator(".map-section")).toBeVisible();

  const layout = await page.evaluate(() => {
    const topbar = document.querySelector("#scenario-controls").getBoundingClientRect();
    const map = document.querySelector(".map-section").getBoundingClientRect();
    return {
      topbarBottom: topbar.bottom,
      mapTop: map.top,
      mapWidth: map.width,
      viewportWidth: window.innerWidth,
      scrollWidth: document.documentElement.scrollWidth,
    };
  });

  expect(layout.mapTop).toBeGreaterThanOrEqual(layout.topbarBottom - 1);
  expect(layout.mapWidth).toBeGreaterThan(layout.viewportWidth * 0.95);
  expect(layout.scrollWidth).toBeLessThanOrEqual(layout.viewportWidth);
});

test("play sidebar lists deployed units and bottom panel inspects selection", async ({ page, isMobile }) => {
  test.skip(isMobile, "desktop-only bottom panel assertion");

  await openPlayMode(page);

  const rosterItems = page.locator(".unit-roster-item");
  await expect(rosterItems.first()).toContainText("Mongol Cavalry");
  await expect(rosterItems.first()).toContainText("Cavalry - HP 10/10");

  await rosterItems.first().click();

  await expect(page.locator(".sidebar-selection-readout")).toHaveText("Mongol Cavalry");
  await expect(page.locator("#unit-name")).toHaveText("Mongol Cavalry");
  await expect(page.locator("#unit-hp")).toHaveText("10/10");
  await expect(page.locator("#unit-move")).toHaveText("4/4");
  await expect(page.locator("#play-details-bar")).toBeVisible();

  const layout = await page.evaluate(() => {
    const sidebar = document.querySelector("#play-controls").getBoundingClientRect();
    const map = document.querySelector(".map-section").getBoundingClientRect();
    const inspector = document.querySelector(".unit-inspector").getBoundingClientRect();
    return {
      sidebarRight: sidebar.right,
      mapLeft: map.left,
      inspectorHeight: inspector.height,
      scrollWidth: document.documentElement.scrollWidth,
      viewportWidth: window.innerWidth,
    };
  });

  expect(layout.mapLeft).toBeGreaterThanOrEqual(layout.sidebarRight - 1);
  expect(layout.inspectorHeight).toBeGreaterThan(0);
  expect(layout.scrollWidth).toBeLessThanOrEqual(layout.viewportWidth);
});

test("mobile play mode keeps the map usable below the unit roster", async ({ page, isMobile }) => {
  test.skip(!isMobile, "mobile-only layout assertion");

  await openPlayMode(page);

  await expect(page.locator("#play-details-bar")).toBeHidden();
  await expect(page.locator(".unit-roster-item")).toHaveCount(4);

  const layout = await page.evaluate(() => {
    const sidebar = document.querySelector("#play-controls").getBoundingClientRect();
    const panel = document.querySelector("#map-panel").getBoundingClientRect();
    return {
      sidebarBottom: sidebar.bottom,
      panelTop: panel.top,
      panelHeight: panel.height,
      scrollWidth: document.documentElement.scrollWidth,
      viewportWidth: window.innerWidth,
    };
  });

  expect(layout.panelTop).toBeGreaterThanOrEqual(layout.sidebarBottom - 1);
  expect(layout.panelHeight).toBeGreaterThan(200);
  expect(layout.scrollWidth).toBeLessThanOrEqual(layout.viewportWidth);
});
