const express = require('express');
const webdriver = require('selenium-webdriver');
const chrome = require('selenium-webdriver/chrome');
const firefox = require('selenium-webdriver/firefox');
const fs = require('fs');

const browser_name = process.argv[2];
const url_path = process.argv[3];
const output_file = process.argv[4];

process.env['PATH'] = `browser_support/node_modules/chromedriver/bin:browser_support/node_modules/geckodriver/bin:${process.env['PATH']}`;

const server = new express();
server.use(express.static('.'));

async function benchmark(port, url_path, output_file) {
  const driver = await new webdriver.Builder()
    .forBrowser(browser_name)
    .setChromeOptions(new chrome.Options().addArguments('--headless'))
    .setFirefoxOptions(new firefox.Options().headless())
    .build();
  try {
    await driver.get(`http://localhost:${port}/${url_path}`);
    await driver.wait(webdriver.until.elementLocated(webdriver.By.id('output')), 60000);
    const output = await driver.findElement(webdriver.By.id('output')).getText();
    await Promise.all([
      fs.promises.writeFile(output_file, output),
      driver.quit()
    ]);
  } catch (exception) {
    await driver.quit();
    throw exception;
  }
}

const listener = server.listen(0, () => {
  benchmark(listener.address().port, url_path, output_file)
    .then(() => process.exit(0))
    .catch(error => {
      console.error(error);
      process.exit(1);
    });
});
