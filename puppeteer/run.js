const express = require('express');
const puppeteer = require('puppeteer');
const fs = require('fs');

const url = process.argv[2];
const output_file = process.argv[3];

const server = new express();
server.use(express.static('.'));

async function benchmark(url, output_file) {
  const browser = await puppeteer.launch();
  const page = await browser.newPage();
  const status = await page.goto(url)
    .then(response => response.status());
  if (status !== 200) {
    browser.close();
    throw `Error ${status} while loading page ${url}`;
  }
  const output = await page.waitForSelector('#output')
    .then(element => page.evaluate(element => element.innerText, element));
  await Promise.all([
    fs.promises.writeFile(output_file, output),
    browser.close()
  ]);
}

server.listen(8080, benchmark(url, output_file)
  .then(() => process.exit(0))
  .catch(error => {
    console.error(error);
    process.exit(1);
  })
);
