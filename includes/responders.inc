<?php

function diginole_feas_webservice_query_processor($args) {

  // Not enough queryable info provided
  if (!array_key_exists('doi', $args) && !array_key_exists('title', $args) && !array_key_exists('authors', $args)) {
    return diginole_feas_webservice_xml_builder('error', 'vague');
  }

  // Request is invalid if no type specified
  else if (array_key_exists('type', $args) && $args['type'] != 'article' && $args['type'] != 'thesis') {
    return diginole_feas_webservice_xml_builder('error', 'type');
  }

  // Respond to valid request
  else {
    $qp = new IslandoraSolrQueryProcessor();

    // Build base queries
    $base_queries = array();
    if (array_key_exists('doi', $args)) {
      $base_queries['mods_identifier_mt'] = $args['doi']; 
    }
    if (array_key_exists('authors', $args)) {
      $base_queries['catch_all_names_mt'] = $args['authors']; 
    }
    if (array_key_exists('title', $args)) {
      $base_queries['catch_all_titles_mt'] = $args['title']; 
    }
    $base_query = "";
    $i = 0;
    foreach ($base_queries as $key => $value) {
      if ($key == 'catch_all_names_mt') {
        $authors = explode('+', str_replace(' ', '+', $value));
        foreach ($authors as $author) {
          $prefix = ($i == 0 ? '' : '+');
          $base_query .= "{$prefix}{$key}:\"{$author}\"";
          $i++;
        }
      }
      else {
        $prefix = ($i == 0 ? "" : "+");
        $base_query .= "{$prefix}{$key}:\"{$value}\"";
        $i++;
      }
    } 
    $qp->buildQuery($base_query);

    // Build filter queries
    $filter_queries = array();
    if (array_key_exists('date', $args)) {
      $filter_queries['mods_originInfo_dateIssued_mt'] = $args['date']; 
    }
    if (array_key_exists('type', $args)) {
      if ($args['type'] == 'article') {
        $filter_queries['RELS_EXT_hasModel_uri_mt'] = "info:fedora/ir:citationCModel";
      }
      if ($args['type'] == 'thesis') {
        $filter_queries['RELS_EXT_hasModel_uri_mt'] = "info:fedora/ir:thesisCModel";
      }
    }
    $filter_query = "";
    $i = 0;
    foreach ($filter_queries as $key => $value) {
      $prefix = ($i = 0 ? "+" : "");
      $filter_query .= "{$prefix}{$key}:\"{$value}\"";
      $i++;
    } 
    $qp->solrParams['fq'] = $filter_query;
    
    $qp->solrLimit = 1000000;
    $qp->solrParams['fl'] = "PID";
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
      $i = 0;
      foreach ($args as $result) {    
        $object = islandora_object_load($result);
        $mods = simplexml_load_string($object['MODS']->content);

        foreach ($mods->identifier as $id) {
          if (strtolower($id['type']) == 'iid') {
            $purl = "http://purl.flvc.org/fsu/fd/{$id}";
            $response->addChild('result', $purl)->addAttribute('pid', $result);
            $i++;
          }
        }
      }
      $response->addAttribute('type', 'success');
      $response->addAttribute('count', $i);
      break;
  }
  return $response->asXML();
}
