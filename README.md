# harbour-sailslack2

Unoffical [Slack](https://slack.com/) client for [Sailfish](https://sailfishos.org).

This is a fork of [Sailslack](https://github.com/danvratil/harbour-sailslack) developed by danvratil.
Sailslack is a fork of [Slackfish](https://github.com/markussammallahti/harbour-slackfish) developed by Markuss Sammallahti.

I do not plan any further development except for fixing bugs preventing me from using it.

## Development

* Install [qpm](https://www.qpm.io/)
* download dependencies ([Qt AsyncFuture](https://github.com/benlau/asyncfuture))
```
qpm install
```

* Create [new Slack application](https://api.slack.com/apps?new_app=1)
and insert its id and secret to file `.qmake.conf`:
```
slack_client_id=xxxxxxxxxxx.xxxxxxxxxxxx
slack_client_secret=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

* Connect to Mer SDK via ssh and, go to Sailslack project directory and build it:
```bash
mb2 -t SailfishOS-2.1.4.13-armv7hl build
```
