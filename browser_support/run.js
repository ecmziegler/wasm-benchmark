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

function timed(promise, timeout) {
  const timeout_error = new Error('Operation timed out');
  const stack = timeout_error.stack.split('\n');
  stack.splice(1,1);
  timeout_error.stack = stack.join('\n');
  return Promise.race([
    promise,
    new Promise((resolve, reject) => setTimeout(() => reject(timeout_error), timeout))
  ]);
}

async function benchmark(port, url_path, output_file) {
  const driver = await timed(new webdriver.Builder()
    .forBrowser(browser_name)
    .setChromeOptions(new chrome.Options().headless())
    .setFirefoxOptions(new firefox.Options().headless())
    .build(), 10000);
  try {
    await timed(driver.get(`http://localhost:${port}/${url_path}`), 5000);
    await timed(driver.wait(webdriver.until.elementLocated(webdriver.By.id('output')), 60000), 60000);
    const output = await timed(driver.findElement(webdriver.By.id('output')).getText(), 5000);
    await timed(Promise.all([
      fs.promises.writeFile(output_file, output),
      driver.quit()
    ]), 5000);
    console.log("File written & browser exited");
  } catch (exception) {
    await timed(driver.quit(), 5000);
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
