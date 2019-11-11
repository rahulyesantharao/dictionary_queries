import json
import string
from collections import defaultdict
import pprint

import numpy as np

from datetime import datetime, timezone
from dateutil.parser import parse

import editdistance

filename = "tweets.json"  # "simdjson/jsonexamples/twitter.json"
if __name__ == "__main__":
    # open data
    data = []
    for i in range(1, 5):
        with open(f"tweets{i}.json") as f:
            for l in f:
                data.append(json.loads(l))

    print(f"** Done opening {len(data)} tweets **")

    # find date range
    dates = sorted([parse(t["created_at"]) for t in data if "created_at" in t])
    print(f"** DATE RANGE: {min(dates)} -> {max(dates)} **")

    # get hashtags
    hashtags = defaultdict(int)
    for tweet in data:
        if "text" in tweet and tweet["text"].find("#") >= 0:
            msg = tweet["text"]
            start = msg.find("#")
            end_indices = list(
                filter(
                    lambda x: x != -1,
                    (
                        msg.find(c, start + 1)
                        for c in string.whitespace + string.punctuation.replace("_", "")
                    ),
                )
            )
            end = min(end_indices) if len(end_indices) > 0 else -1
            if len(msg[start:end]) > 1:
                hashtags[msg[start:end]] += 1

    hashtag_groups = defaultdict(list)
    for hashtag in hashtags:
        inserted = False
        for rep in hashtag_groups:
            maxlen = max(len(rep), len(hashtag))
            editd = editdistance.eval(rep, hashtag)
            if (4 <= maxlen < 10 and editd < 2) or (
                maxlen >= 10 and editd / maxlen < 0.1
            ):
                hashtag_groups[rep].append(hashtag)
                inserted = True
                break
        if not inserted:
            hashtag_groups[hashtag] = []
    pprint.pprint(hashtag_groups)

    # sort hashtags
    hashtag_nums = []
    for rep in hashtag_groups:
        x = (rep,) + tuple(hashtag_groups[rep])
        num = 0
        for h in x:
            num += hashtags[h]
        hashtag_nums.append((num, x))
    print(hashtag_nums)

    sorted_hashtags = sorted(hashtag_nums)[::-1]
    threshold = 10
    popular_hashtags = list(filter(lambda pair: pair[0] > threshold, sorted_hashtags))
    print(f"\n** POPULAR HASHTAGS (> {threshold} occurrences) **")
    pprint.pprint(popular_hashtags)

    # who used what hashtags
    print("\n** FINDING DATA ON POPULAR HASHTAGS **")
    users_by_hashtag = defaultdict(lambda: defaultdict(int))
    dates_by_hashtag = defaultdict(list)
    for tweet in data:
        for _, hashtags in popular_hashtags:
            for hashtag in hashtags:
                if "text" in tweet and hashtag in tweet["text"]:
                    users_by_hashtag[hashtags][tweet["user"]["id"]] += 1
                    dates_by_hashtag[hashtags].append(parse(tweet["created_at"]))

    # find hashtags where users use it more than once
    print("\n** USER USES OF POPULAR HASHTAGS **")
    for num, hashtags in popular_hashtags:
        users = sorted(
            ((num, user) for user, num in users_by_hashtag[hashtags].items() if num > 1)
        )
        if len(users) > 0:
            print(f"{num}: {hashtags}:\n - {users}")

    # find time importance
    print("\n** POPULAR HASHTAG TIME MEASURES **")
    for num, hashtags in popular_hashtags:
        med_time = datetime.fromtimestamp(
            np.median(list(map(datetime.timestamp, dates_by_hashtag[hashtags])))
        )
        mean_time = datetime.fromtimestamp(
            np.mean(list(map(datetime.timestamp, dates_by_hashtag[hashtags])))
        )

        med_time_local = datetime.strftime(
            med_time.replace(tzinfo=timezone.utc).astimezone(tz=None),
            "%b %d, %Y %I:%M:%S %p",
        )
        mean_time_local = datetime.strftime(
            mean_time.replace(tzinfo=timezone.utc).astimezone(tz=None),
            "%b %d, %Y %I:%M:%S %p",
        )

        print(
            f"{num}: {hashtags}:\n - Median: {med_time_local}\n - Mean: {mean_time_local}"
        )
