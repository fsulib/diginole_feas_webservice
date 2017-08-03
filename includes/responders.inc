<?php

function diginole_feas_webservice_query_processor($args) {

  // Request is invalid if no type specified
  if (array_key_exists('type', $args) && $args['type'] != 'article' && $args['type'] != 'thesis') {
    return diginole_feas_webservice_xml_builder('error', 'type');
  }
 
  // Respond to valid request
  else {
    $qp = new IslandoraSolrQueryProcessor();
    
    if (array_key_exists('doi', $args)) {
      $query = "mods_identifier_mt:\"{$args['doi']}\"";
      $qp->buildQuery($query);
      if (array_key_exists('authors', $args)) {
        $qp->solrParams['fq'] = "catch_all_names_mt:\"{$args['authors']}\"";
      }
      if (array_key_exists('title', $args)) {
        $qp->solrParams['fq'] = "catch_all_titles_mt:\"{$args['title']}\"";
      }
    }
    else if (array_key_exists('authors', $args)) {
      watchdog("dn", $args['authors']);
      $query = "catch_all_names_mt:\"{$args['authors']}\"";
      $qp->buildQuery($query);
      if (array_key_exists('title', $args)) {
        $qp->solrParams['fq'] = "catch_all_titles_mt:\"{$args['title']}\"";
      }
    }
    else if (array_key_exists('title', $args)) {
      watchdog("dn", $args['title']);
      $query = "catch_all_titles_mt:\"{$args['title']}\"";
      $qp->buildQuery($query);
    }
    else {
      return diginole_feas_webservice_xml_builder('error', 'vague');
    }
    
    $qp->solrParams['fl'] = "PID";

    if (array_key_exists('date', $args)) {
      $qp->solrParams['fq'] = "mods_originInfo_dateIssued_mt:\"{$args['date']}\"";
    }

    if (array_key_exists('type', $args)) {
      if ($args['type'] == 'article') {
        $qp->solrParams['fq'] = "RELS_EXT_hasModel_uri_mt:\"info:fedora/ir:citationCModel\"";
      }
      if ($args['type'] == 'thesis') {
        $qp->solrParams['fq'] = "RELS_EXT_hasModel_uri_mt:\"info:fedora/ir:thesisCModel\"";
      }
    }

    $qp->executeQuery(FALSE);
    $results = $qp->islandoraSolrResult['response'];
    $pids = array();
    foreach ($results['objects'] as $result) {
      $pids[] = $result['solr_doc']['PID'];
    }
   
    return diginole_feas_webservice_xml_builder('success', $pids);

  }

}

function diginole_feas_webservice_xml_builder($type, $args) {
  $response = new SimpleXMLElement("<response></response>");
  switch ($type) {
    case 'error':
      $response->addAttribute('type', 'error');
        switch ($args) {
          case 'type':
            $response->addChild('error', 'Provided type is not supported. Available options are \'thesis\' and \'article\'.');
            break;
          case 'vague':
            $response->addChild('error', 'Not enough information provided. Please supply a \'doi\', \'authors\' or at least \'title\'.');
            break;
        }
      break;
    case 'success':
      $response->addAttribute('type', 'success');
      $response->addAttribute('count', count($args));
      foreach ($args as $result) {    
        $object = islandora_object_load($result);
        $mods = simplexml_load_string($object['MODS']->content);
        $purl = $mods->location->url;
        $response->addChild('result', $purl)->addAttribute('pid', $result);
      }
      break;
  }
  return $response->asXML();
}
