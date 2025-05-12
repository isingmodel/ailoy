const { execSync } = require("child_process");
const cpus = require("os").cpus().length;

const cmd = `cmake-js build -d ../.. -O build --CDNODE:BOOL=ON --CDAILOY_WITH_TEST:BOOL=OFF --parallel ${cpus}`;
execSync(cmd, { stdio: "inherit", shell: true });
