.pragma library

/**
   Copyright 2018 June Hanabi

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

.import "../../libs/lodash.js" as Lodash
const _ = Lodash._;

class TextSearch
{
  constructor(keys = []) {
    this.keys = keys;
  }

  doFilter(prop, inclusive) {
    this.keys = _.filter(this.keys, (value) => {
                           if (inclusive && value[prop])
                           return true;

                           else if (!inclusive && !value[prop])
                           return true;

                           return false;
                         });

    return this;
  }

  doInclusiveFilter(prop) {
    return this.doFilter(prop, true);
  }

  doExclusiveFilter(prop) {
    return this.doFilter(prop, false);
  }

  get andShorthand() {
    return this.doInclusiveFilter("shorthand");
  }

  get notShorthand() {
    return this.doExclusiveFilter("shorthand");
  }

  get andNormal() {
    return this.doInclusiveFilter("normal");
  }

  get notNormal() {
    return this.doExclusiveFilter("normal");
  }

  get andControl() {
    return this.doInclusiveFilter("control");
  }

  get notControl() {
    return this.doExclusiveFilter("control");
  }

  get andPic() {
    return this.doInclusiveFilter("picture");
  }

  get notPic() {
    return this.doExclusiveFilter("picture");
  }

  get andSingle() {
    return this.doInclusiveFilter("singleChar");
  }

  get notSingle() {
    return this.doExclusiveFilter("singleChar");
  }

  get andMulti() {
    return this.doInclusiveFilter("multiChar");
  }

  get notMulti() {
    return this.doExclusiveFilter("multiChar");
  }

  get andVar() {
    return this.doInclusiveFilter("variable");
  }

  get notVar() {
    return this.doExclusiveFilter("variable");
  }

  get andTiles() {
    return this.doInclusiveFilter("useTilemap");
  }

  get notTiles() {
    return this.doExclusiveFilter("useTilemap");
  }
}
